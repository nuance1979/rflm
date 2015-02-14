/*
 * MultipassNgramDTLM.cc --
 *      Multi-pass decision tree language models based on ngram stats
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include "MultipassNgramDTLM.h"

#define MTREE ((MultipassNgramDT<CountType> *)tree)

/*
 * Debug levels used
 */
#define DEBUG_ESTIMATE_WARNINGS 1
#define DEBUG_FIXUP_WARNINGS 3
#define DEBUG_PRINT_GTPARAMS 2
#define DEBUG_READ_STATS 1
#define DEBUG_WRITE_STATS 1
#define DEBUG_NGRAM_HITS 2
#define DEBUG_ESTIMATES 4

MultipassNgramDTLM::MultipassNgramDTLM(Vocab &vocab,
				       MultipassNgramDT<NgramCount> &dt,
				       Discount &dist)
  : NgramDecisionTreeLM(vocab, dt, dist)
{
}

MultipassNgramDTLM::~MultipassNgramDTLM()
{
}

LogP
MultipassNgramDTLM::wordProb(VocabIndex word, const VocabIndex *context)
{
  if (estimated) return Ngram::wordProb(word, context);
  else {
    cerr << "model hasn't been estimated.\n";
    exit(1);
  }
}

Boolean
MultipassNgramDTLM::estimate(NgramStats &stats, Discount **discounts)
{
  return estimate2(stats, discounts);
}

/*
 * Generic version of estimate(NgramStats, Discount)
 *                and estimate(NgramCounts<FloatCount>, Discount)
 *
 * XXX: This function is essentially duplicated in other places.
 * Propate changes to VarNgram::estimate().
 *
 * Based on Ngram::estimate2(NgramCounts<CountType> &stats, 
 *                           Discount **discounts)
 *
 */

template <class CountType>
Boolean
MultipassNgramDTLM::estimate2(NgramCounts<CountType> &stats,
			      Discount **discounts)
{
  // Compute ngrams using tree
  assert(tree);
  MTREE->eval(stats, *this);
  
  // Use bailout LM to compute the rest

    /*
     * For all ngrams, compute probabilities and apply the discount
     * coefficients.
     */
    VocabIndex context[order];
    unsigned vocabSize = Ngram::vocabSize();

    unsigned internalOrder = MTREE->getInternalOrder();
    /*
     * Remove all old contexts
     */
    // NOTE: don't do that in this case!!!
    //    clear();

    /*
     * Ensure <s> unigram exists (being a non-event, it is not inserted
     * in distributeProb(), yet is assumed by much other software).
     */
    if (vocab.ssIndex() != Vocab_None) {
	context[0] = Vocab_None;
	*insertProb(vocab.ssIndex(), context) = LogP_Zero;
    }

    for (unsigned i = 1; i <= order; i++) {
	unsigned noneventContexts = 0;
	unsigned noneventNgrams = 0;
	unsigned discountedNgrams = 0;

	/*
	 * check if discounting is disabled for this round
	 */
	Boolean noDiscount =
			(discounts == 0) ||
			(discounts[i-1] == 0) ||
			discounts[i-1]->nodiscount();

	Boolean interpolate =
			(discounts != 0) &&
			(discounts[i-1] != 0) &&
			discounts[i-1]->interpolate;

	/*
	 * modify counts are required by discounting method
	 */
	if (!noDiscount && discounts && discounts[i-1]) {
	    discounts[i-1]->prepareCounts(stats, i, order);
	}

	/*
	 * This enumerates all contexts, i.e., i-1 grams.
	 */
	CountType *contextCount;
	NgramCountsIter<CountType> contextIter(stats, context, i-1);

	while (contextCount = contextIter.next()) {
	    /*
	     * Skip contexts ending in </s>.  This typically only occurs
	     * with the doubling of </s> to generate trigrams from
	     * bigrams ending in </s>.
	     * If <unk> is not real word, also skip context that contain
	     * it.
	     */
	  if (i > 1 && context[i-2] == vocab.seIndex() ||
	        vocab.isNonEvent(vocab.unkIndex()) &&
				 vocab.contains(context, vocab.unkIndex()))
	    {
		noneventContexts ++;
		continue;
	    }

	    VocabIndex word[2];	/* the follow word */
	    NgramCountsIter<CountType> followIter(stats, context, word, 1);
	    CountType *ngramCount;

	    /*
	     * Total up the counts for the denominator
	     * (the lower-order counts may not be consistent with
	     * the higher-order ones, so we can't just use *contextCount)
	     * Only if the trustTotal flag is set do we override this
	     * with the count from the context ngram.
	     */
	    CountType totalCount = 0;
	    Count observedVocab = 0, min2Vocab = 0, min3Vocab = 0;
	    while (ngramCount = followIter.next()) {
		if (vocab.isNonEvent(word[0]) ||
		    ngramCount == 0 ||
		    i == 1 && vocab.isMetaTag(word[0]))
		{
		    continue;
		}

		if (!vocab.isMetaTag(word[0])) {
		    totalCount += *ngramCount;
		    observedVocab ++;
		    if (*ngramCount >= 2) {
			min2Vocab ++;
		    }
		    if (*ngramCount >= 3) {
			min3Vocab ++;
		    }
		} else {
		    /*
		     * Process meta-counts
		     */
		    unsigned type = vocab.typeOfMetaTag(word[0]);
		    if (type == 0) {
			/*
			 * a count total: just add to the totalCount
			 * the corresponding type count can't be known,
			 * but it has to be at least 1
			 */
			totalCount += *ngramCount;
			observedVocab ++;
		    } else {
			/*
			 * a count-of-count: increment the word type counts,
			 * and infer the totalCount
			 */
			totalCount += type * *ngramCount;
			observedVocab += (Count)*ngramCount;
			if (type >= 2) {
			    min2Vocab += (Count)*ngramCount;
			}
			if (type >= 3) {
			    min3Vocab += (Count)*ngramCount;
			}
		    }
		}
	    }

	    if (i > 1 && trustTotals()) {
		totalCount = *contextCount;
	    }

	    if (totalCount == 0) {
		continue;
	    }

	    /*
	     * reverse the context ngram since that's how
	     * the BO nodes are indexed.
	     */
	    Vocab::reverse(context);

	    /*
	     * Compute the discounted probabilities
	     * from the counts and store them in the backoff model.
	     */
	retry:
	    followIter.init();
	    Prob totalProb = 0.0;

	    while (ngramCount = followIter.next()) {
		LogP lprob;
		double discount;

		/*
		 * Ignore zero counts.
		 * They are there just as an artifact of the count trie
		 * if a higher order ngram has a non-zero count.
		 */
		if (i > 1 && *ngramCount == 0) {
		  continue;
		}

		if (vocab.isNonEvent(word[0]) || vocab.isMetaTag(word[0])) {
		    /*
		     * Discard all pseudo-word probabilities,
		     * except for unigrams.  For unigrams, assign
		     * probability zero.  This will leave them with
		     * prob zero in all cases, due to the backoff
		     * algorithm.
		     * Also discard the <unk> token entirely in closed
		     * vocab models, its presence would prevent OOV
		     * detection when the model is read back in.
		     */
		    if (i > 1 ||
			word[0] == vocab.unkIndex() ||
			vocab.isMetaTag(word[0]))
		    {
			noneventNgrams ++;
			continue;
		    }

		    lprob = LogP_Zero;
		    discount = 1.0;
		} else {

		  // Skip ngrams which have been computed by tree
		  if (i == order && findProb(word[0], context) != 0)
		    continue;

		  // Bailout language model in action
		  lprob = tree->getBackoffLM().wordProb(word[0], context);
		  Prob prob = LogPtoProb(lprob); assert(prob > 0.0);
		  File *file = MTREE->getInfoDumpFile();
		  if (file != 0) { // Output detailed info for ngram comput.
		    if (i == order) 
		      file->fprintf("unseen_history");
		    else 
		      file->fprintf("lower_order");
		    for (int j=i-2; j>=0; j--)
		      file->fprintf(" %s", vocab.getWord(context[j]));
		    file->fprintf(" %s", vocab.getWord(word[0]));
		    file->fprintf(" %e\n", prob);
		  }

		  double lowerOrderWeight;
		  LogP lowerOrderProb;
		  discount = 1.0; // dummy value

		    if (discount != 0.0) {
			totalProb += prob;
		    }

		    if (discount != 0.0 && debug(DEBUG_ESTIMATES)) {
			dout() << "CONTEXT " << (vocab.use(), context)
			       << " WORD " << vocab.getWord(word[0])
			       << " NUMER " << *ngramCount
			       << " DENOM " << totalCount
			       << " DISCOUNT " << discount;

			if (interpolate) {
			    dout() << " LOW " << lowerOrderWeight
				   << " LOLPROB " << lowerOrderProb;
			}
			dout() << " LPROB " << lprob << endl;
		    }
		}
		    
		/*
		 * A discount coefficient of zero indicates this ngram
		 * should be omitted entirely (presumably to save space).
		 */
		if (discount == 0.0) {
		    discountedNgrams ++;
		    removeProb(word[0], context);
		} else {
		  *insertProb(word[0], context) = lprob;
		} 
	    }

	    /*
	     * This is a hack credited to Doug Paul (by Roni Rosenfeld in
	     * his CMU tools).  It may happen that no probability mass
	     * is left after totalling all the explicit probs, typically
	     * because the discount coefficients were out of range and
	     * forced to 1.0.  To arrive at some non-zero backoff mass
	     * we try incrementing the denominator in the estimator by 1.
	     */
	    if (!noDiscount && totalCount > 0 &&
		totalProb > 1.0 - Prob_Epsilon)
	    {
		totalCount ++;

		if (debug(DEBUG_ESTIMATE_WARNINGS)) {
		    cerr << "warning: " << (1.0 - totalProb)
			 << " backoff probability mass left for \""
			 << (vocab.use(), context)
			 << "\" -- incrementing denominator"
			 << endl;
		}
		/* hack disabled for decision tree language model
		 * because for ${order}-gram we don't need prob mass
		 * reserved for back off (always interpolate)
		 */
		//goto retry;
	    }

	    /*
	     * Undo the reversal above so the iterator can continue correctly
	     */
	    Vocab::reverse(context);
	}

	if (debug(DEBUG_ESTIMATE_WARNINGS)) {
	    if (noneventContexts > 0) {
		dout() << "discarded " << noneventContexts << " "
		       << i << "-gram contexts containing pseudo-events\n";
	    }
	    if (noneventNgrams > 0) {
		dout() << "discarded " << noneventNgrams << " "
		       << i << "-gram probs predicting pseudo-events\n";
	    }
	    if (discountedNgrams > 0) {
		dout() << "discarded " << discountedNgrams << " "
		       << i << "-gram probs discounted to zero\n";
	    }
	}

	/*
	 * With all the probs in place, BOWs are obtained simply by the usual
	 * normalization.
	 * We do this right away before computing probs of higher order since 
	 * the estimation of higher-order N-grams can refer to lower-order
	 * ones (e.g., for interpolated estimates).
	 */
	computeBOWs(i-1);
    }

    fixupProbs();

    estimated = true;
    
    return true;
}
