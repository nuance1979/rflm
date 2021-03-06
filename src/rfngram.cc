/*
 * rfngram --
 *	Create and manipulate random forest models
 *
 * Yi Su <suy@jhu.edu>
 *      based on SRI LM Toolkit's ngram.cc
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2004 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Id: rfngram.cc,v 1.3 2009/04/06 18:28:01 yisu Exp $";
#endif

#ifdef PRE_ISO_CXX
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

extern "C" {
    void srand48(long);           /* might be missing from math.h or stdlib.h */
}

#include "option.h"
#include "version.h"
#include "zio.h"
#include "File.h"
#include "Vocab.h"
#include "SubVocab.h"
#include "MultiwordVocab.h"
#include "MultiwordLM.h"
#include "NBest.h"
#include "TaggedVocab.h"
#include "Ngram.h"
#include "TaggedNgram.h"
#include "StopNgram.h"
#include "ClassNgram.h"
#include "SimpleClassNgram.h"
#include "DFNgram.h"
#include "SkipNgram.h"
#include "HiddenNgram.h"
#include "HiddenSNgram.h"
#include "NullLM.h"
#include "BayesMix.h"
#include "AdaptiveMix.h"
#include "AdaptiveMarginals.h"
#include "CacheLM.h"
#include "DynamicLM.h"
#include "DecipherNgram.h"
#include "HMMofNgrams.h"
#include "RefList.h"
#include "ProductNgram.h"

#include "NgramDecisionTree.h"
#include "MultipassNgramDT.h"
#include "Discount.h"
#include "NgramDecisionTreeLM.h"
#include "MultipassNgramDTLM.h"
#include "LMServer.h"
#include "MultipassNgramDT.h"

const unsigned maxorder = 9;

static int version = 0;
static unsigned order = defaultNgramOrder;
static unsigned debug = 0;
static char *pplFile = 0;
static char *escape = 0;
static char *countFile = 0;
static int countEntropy = 0;
static unsigned countOrder = 0;
static char *vocabFile = 0;
static char *noneventFile = 0;
static int limitVocab = 0;
static char *lmFile  = 0;
static char *mixFile  = 0;
static char *mixFile2 = 0;
static char *mixFile3 = 0;
static char *mixFile4 = 0;
static char *mixFile5 = 0;
static char *mixFile6 = 0;
static char *mixFile7 = 0;
static char *mixFile8 = 0;
static char *mixFile9 = 0;
static int bayesLength = -1;	/* marks unset option */
static double bayesScale = 1.0;
static double mixLambda = 0.5;
static double mixLambda2 = 0.0;
static double mixLambda3 = 0.0;
static double mixLambda4 = 0.0;
static double mixLambda5 = 0.0;
static double mixLambda6 = 0.0;
static double mixLambda7 = 0.0;
static double mixLambda8 = 0.0;
static double mixLambda9 = 0.0;
static int reverse1 = 0;
static char *writeLM  = 0;
static char *writeVocab  = 0;
static int memuse = 0;
static int renormalize = 0;
static double prune = 0.0;
static int pruneLowProbs = 0;
static int minprune = 2;
static int skipOOVs = 0;
static unsigned generate1 = 0;
static int seed = 0;  /* default dynamically generated in main() */
static int df = 0;
static int skipNgram = 0;
static int hiddenS = 0;
static char *hiddenVocabFile = 0;
static int hiddenNot = 0;
static char *classesFile = 0;
static int simpleClasses = 0;
static int expandClasses = -1;
static unsigned expandExact = 0;
static int tagged = 0;
static int factored = 0;
static int toLower = 0;
static int multiwords = 0;
static int splitMultiwords = 0;
static int keepunk = 0;
static int keepnull = 1;
static char *mapUnknown = 0;
static int null = 0;
static unsigned cache = 0;
static double cacheLambda = 0.05;
static int dynamic = 0;
static double dynamicLambda = 0.05;
static char *noiseTag = 0;
static char *noiseVocabFile = 0;
static char *stopWordFile = 0;
static int decipherHack = 0;
static int hmm = 0;
static int adaptMix = 0;
static double adaptDecay = 1.0;
static unsigned adaptIters = 2;
static char *adaptMarginals = 0;
static double adaptMarginalsBeta = 0.5;
static int adaptMarginalsRatios = 0;
static char *baseMarginals = 0;
static char *rescoreNgramFile = 0;

/*
 * N-Best related variables
 */

static char *nbestFile = 0;
static char *nbestFiles = 0;
static char *writeNbestDir = 0;
static int writeDecipherNbest = 0;
static int noReorder = 0;
static unsigned maxNbest = 0;
static char *rescoreFile = 0;
static char *decipherLM = 0;
static unsigned decipherOrder = 2;
static int decipherNoBackoff = 0;
static double decipherLMW = 8.0;
static double decipherWTW = 0.0;
static double rescoreLMW = 8.0;
static double rescoreWTW = 0.0;

/*
 * Decision tree related
 */

static char *dtFile = 0;
static char *dtCounts = 0;
static char *dtTest = 0;
static char *heldoutFile = 0;
static char *heldoutCount = 0;
static char *trainCountsFile = 0;
static char *trainTextFile = 0;
static int runServer = 0;
static int serverPort = 30000;
static unsigned mixNum = 1;
static char *mixList = 0;
static char *mixParam = 0;
static int ukndiscount = 0;
static char *knFile[maxorder+1];
static int readWithMincounts = 0;
static unsigned gtmin[maxorder+1] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2};
static unsigned gtmax[maxorder+1] = {5, 1, 7, 7, 7, 7, 7, 7, 7, 7};
static char *dumpLeaf = 0;
static unsigned internalOrder = 0;
static char *ngramDump = 0;
static char *infoDump = 0;

static Option options[] = {
    { OPT_TRUE, "version", &version, "print version information" },
    { OPT_UINT, "order", &order, "max ngram order" },
    { OPT_UINT, "debug", &debug, "debugging level for lm" },
    { OPT_TRUE, "skipoovs", &skipOOVs, "skip n-gram contexts containing OOVs" },
    { OPT_TRUE, "df", &df, "use disfluency ngram model" },
    { OPT_TRUE, "tagged", &tagged, "use a tagged LM" },
    { OPT_TRUE, "factored", &factored, "use a factored LM" },
    { OPT_TRUE, "skip", &skipNgram, "use skip ngram model" },
    { OPT_TRUE, "hiddens", &hiddenS, "use hidden sentence ngram model" },
    { OPT_STRING, "hidden-vocab", &hiddenVocabFile, "hidden ngram vocabulary" },
    { OPT_TRUE, "hidden-not", &hiddenNot, "process overt hidden events" },
    { OPT_STRING, "classes", &classesFile, "class definitions" },
    { OPT_TRUE, "simple-classes", &simpleClasses, "use unique class model" },
    { OPT_INT, "expand-classes", &expandClasses, "expand class-model into word-model" },
    { OPT_UINT, "expand-exact", &expandExact, "compute expanded ngrams longer than this exactly" },
    { OPT_STRING, "stop-words", &stopWordFile, "stop-word vocabulary for stop-Ngram LM" },
    { OPT_TRUE, "decipher", &decipherHack, "use bigram model exactly as recognizer" },
    { OPT_TRUE, "unk", &keepunk, "vocabulary contains unknown word tag" },
    { OPT_FALSE, "nonull", &keepnull, "remove <NULL> in LM" },
    { OPT_STRING, "map-unk", &mapUnknown, "word to map unknown words to" },
    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_TRUE, "multiwords", &multiwords, "split multiwords for LM evaluation" },
    { OPT_STRING, "ppl", &pplFile, "text file to compute perplexity from" },
    { OPT_STRING, "escape", &escape, "escape prefix to pass data through -ppl" },
    { OPT_STRING, "counts", &countFile, "count file to compute perplexity from" },
    { OPT_TRUE, "counts-entropy", &countEntropy, "compute entropy (not perplexity) from counts" },
    { OPT_UINT, "count-order", &countOrder, "max count order used by -counts" },
    { OPT_UINT, "gen", &generate1, "number of random sentences to generate" },
    { OPT_INT, "seed", &seed, "seed for randomization" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_STRING, "nonevents", &noneventFile, "non-event vocabulary" },
    { OPT_TRUE, "limit-vocab", &limitVocab, "limit LM reading to specified vocabulary" },
    { OPT_STRING, "lm", &lmFile, "file in ARPA LM format" },
    { OPT_UINT, "bayes", &bayesLength, "context length for Bayes mixture LM" },
    { OPT_FLOAT, "bayes-scale", &bayesScale, "log likelihood scale for -bayes" },
    { OPT_STRING, "mix-lm", &mixFile, "LM to mix in" },
    { OPT_FLOAT, "lambda", &mixLambda, "mixture weight for -lm" },
    { OPT_STRING, "mix-lm2", &mixFile2, "second LM to mix in" },
    { OPT_FLOAT, "mix-lambda2", &mixLambda2, "mixture weight for -mix-lm2" },
    { OPT_STRING, "mix-lm3", &mixFile3, "third LM to mix in" },
    { OPT_FLOAT, "mix-lambda3", &mixLambda3, "mixture weight for -mix-lm3" },
    { OPT_STRING, "mix-lm4", &mixFile4, "fourth LM to mix in" },
    { OPT_FLOAT, "mix-lambda4", &mixLambda4, "mixture weight for -mix-lm4" },
    { OPT_STRING, "mix-lm5", &mixFile5, "fifth LM to mix in" },
    { OPT_FLOAT, "mix-lambda5", &mixLambda5, "mixture weight for -mix-lm5" },
    { OPT_STRING, "mix-lm6", &mixFile6, "sixth LM to mix in" },
    { OPT_FLOAT, "mix-lambda6", &mixLambda6, "mixture weight for -mix-lm6" },
    { OPT_STRING, "mix-lm7", &mixFile7, "seventh LM to mix in" },
    { OPT_FLOAT, "mix-lambda7", &mixLambda7, "mixture weight for -mix-lm7" },
    { OPT_STRING, "mix-lm8", &mixFile8, "eighth LM to mix in" },
    { OPT_FLOAT, "mix-lambda8", &mixLambda8, "mixture weight for -mix-lm8" },
    { OPT_STRING, "mix-lm9", &mixFile9, "ninth LM to mix in" },
    { OPT_FLOAT, "mix-lambda9", &mixLambda9, "mixture weight for -mix-lm9" },
    { OPT_TRUE, "null", &null, "use a null language model" },
    { OPT_UINT, "cache", &cache, "history length for cache language model" },
    { OPT_FLOAT, "cache-lambda", &cacheLambda, "interpolation weight for -cache" },
    { OPT_TRUE, "dynamic", &dynamic, "interpolate with a dynamic lm" },
    { OPT_TRUE, "hmm", &hmm, "use HMM of n-grams model" },
    { OPT_TRUE, "adapt-mix", &adaptMix, "use adaptive mixture of n-grams model" },
    { OPT_FLOAT, "adapt-decay", &adaptDecay, "history likelihood decay factor" },
    { OPT_UINT, "adapt-iters", &adaptIters, "EM iterations for adaptive mix" },
    { OPT_STRING, "adapt-marginals", &adaptMarginals, "unigram marginals to adapt base LM to" },
    { OPT_STRING, "base-marginals", &baseMarginals, "unigram marginals of base LM to" },
    { OPT_FLOAT, "adapt-marginals-beta", &adaptMarginalsBeta, "marginals adaptation weight" },
    { OPT_TRUE, "adapt-marginals-ratios", &adaptMarginalsRatios, "compute ratios between marginals-adapted and base probs" },
    { OPT_FLOAT, "dynamic-lambda", &dynamicLambda, "interpolation weight for -dynamic" },
    { OPT_TRUE, "reverse", &reverse1, "reverse words" },
    { OPT_STRING, "rescore-ngram", &rescoreNgramFile, "recompute probs in N-gram LM" },
    { OPT_STRING, "write-lm", &writeLM, "re-write LM to file" },
    { OPT_STRING, "write-vocab", &writeVocab, "write LM vocab to file" },
    { OPT_TRUE, "renorm", &renormalize, "renormalize backoff weights" },
    { OPT_FLOAT, "prune", &prune, "prune redundant probs" },
    { OPT_UINT, "minprune", &minprune, "prune only ngrams at least this long" },
    { OPT_TRUE, "prune-lowprobs", &pruneLowProbs, "low probability N-grams" },

    { OPT_TRUE, "memuse", &memuse, "show memory usage" },

    { OPT_STRING, "nbest", &nbestFile, "nbest list file to rescore" },
    { OPT_STRING, "nbest-files", &nbestFiles, "list of N-best filenames" },
    { OPT_TRUE, "split-multiwords", &splitMultiwords, "split multiwords in N-best lists" },
    { OPT_STRING, "write-nbest-dir", &writeNbestDir, "output directory for N-best rescoring" },
    { OPT_TRUE, "decipher-nbest", &writeDecipherNbest, "output Decipher n-best format" },
    { OPT_UINT, "max-nbest", &maxNbest, "maximum number of hyps to consider" },
    { OPT_TRUE, "no-reorder", &noReorder, "don't reorder N-best hyps after rescoring" },
    { OPT_STRING, "rescore", &rescoreFile, "hyp stream input file to rescore" },
    { OPT_STRING, "decipher-lm", &decipherLM, "DECIPHER(TM) LM for nbest list generation" },
    { OPT_UINT, "decipher-order", &decipherOrder, "ngram order for -decipher-lm" },
    { OPT_TRUE, "decipher-nobackoff", &decipherNoBackoff, "disable backoff hack in recognizer LM" },
    { OPT_FLOAT, "decipher-lmw", &decipherLMW, "DECIPHER(TM) LM weight" },
    { OPT_FLOAT, "decipher-wtw", &decipherWTW, "DECIPHER(TM) word transition weight" },
    { OPT_FLOAT, "rescore-lmw", &rescoreLMW, "rescoring LM weight" },
    { OPT_FLOAT, "rescore-wtw", &rescoreWTW, "rescoring word transition weight" },
    { OPT_STRING, "noise", &noiseTag, "noise tag to skip" },
    { OPT_STRING, "noise-vocab", &noiseVocabFile, "noise vocabulary to skip" },

    { OPT_UINT, "gtmin", &gtmin[0], "lower GT discounting cutoff" },
    { OPT_UINT, "gtmax", &gtmax[0], "upper GT discounting cutoff" },
    { OPT_UINT, "gt1min", &gtmin[1], "lower 1gram discounting cutoff" },
    { OPT_UINT, "gt1max", &gtmax[1], "upper 1gram discounting cutoff" },
    { OPT_UINT, "gt2min", &gtmin[2], "lower 2gram discounting cutoff" },
    { OPT_UINT, "gt2max", &gtmax[2], "upper 2gram discounting cutoff" },
    { OPT_UINT, "gt3min", &gtmin[3], "lower 3gram discounting cutoff" },
    { OPT_UINT, "gt3max", &gtmax[3], "upper 3gram discounting cutoff" },
    { OPT_UINT, "gt4min", &gtmin[4], "lower 4gram discounting cutoff" },
    { OPT_UINT, "gt4max", &gtmax[4], "upper 4gram discounting cutoff" },
    { OPT_UINT, "gt5min", &gtmin[5], "lower 5gram discounting cutoff" },
    { OPT_UINT, "gt5max", &gtmax[5], "upper 5gram discounting cutoff" },
    { OPT_UINT, "gt6min", &gtmin[6], "lower 6gram discounting cutoff" },
    { OPT_UINT, "gt6max", &gtmax[6], "upper 6gram discounting cutoff" },
    { OPT_UINT, "gt7min", &gtmin[7], "lower 7gram discounting cutoff" },
    { OPT_UINT, "gt7max", &gtmax[7], "upper 7gram discounting cutoff" },
    { OPT_UINT, "gt8min", &gtmin[8], "lower 8gram discounting cutoff" },
    { OPT_UINT, "gt8max", &gtmax[8], "upper 8gram discounting cutoff" },
    { OPT_UINT, "gt9min", &gtmin[9], "lower 9gram discounting cutoff" },
    { OPT_UINT, "gt9max", &gtmax[9], "upper 9gram discounting cutoff" },

    { OPT_STRING, "kn", &knFile[0], "Kneser-Ney discount parameter file" },
    { OPT_STRING, "kn1", &knFile[1], "Kneser-Ney 1gram discounts" },
    { OPT_STRING, "kn2", &knFile[2], "Kneser-Ney 2gram discounts" },
    { OPT_STRING, "kn3", &knFile[3], "Kneser-Ney 3gram discounts" },
    { OPT_STRING, "kn4", &knFile[4], "Kneser-Ney 4gram discounts" },
    { OPT_STRING, "kn5", &knFile[5], "Kneser-Ney 5gram discounts" },
    { OPT_STRING, "kn6", &knFile[6], "Kneser-Ney 6gram discounts" },
    { OPT_STRING, "kn7", &knFile[7], "Kneser-Ney 7gram discounts" },
    { OPT_STRING, "kn8", &knFile[8], "Kneser-Ney 8gram discounts" },
    { OPT_STRING, "kn9", &knFile[9], "Kneser-Ney 9gram discounts" },
    { OPT_TRUE, "ukndiscount", &ukndiscount, "use original Kneser-Ney discounting" },

    { OPT_STRING, "dt", &dtFile, "decision tree file" },
    { OPT_STRING, "dt-counts", &dtCounts, "multipass decision tree intermediate counts" },
    { OPT_STRING, "dt-test", &dtTest, "multipass decision tree intermediate test counts" },
    { OPT_STRING, "heldout-text", &heldoutFile, "heldout text file" },
    { OPT_STRING, "heldout-read", &heldoutCount, "heldout counts file" },
    { OPT_STRING, "read", &trainCountsFile, "training counts file" },
    { OPT_TRUE, "read-with-mincounts", &readWithMincounts, "apply minimum counts when reading counts file" },
    { OPT_STRING, "text", &trainTextFile, "training text file" },
    { OPT_TRUE, "server", &runServer, "running LM server" },
    { OPT_INT, "port", &serverPort, "port on which LM server runs" },
    { OPT_UINT, "mix-num", &mixNum, "number of LMs to mix" },
    { OPT_STRING, "mix-list", &mixList, "list of LMs to mix" },
    { OPT_STRING, "mix-param", &mixParam, "list of lambdas for mixing LMs; default 1/mix-num" },
    { OPT_STRING, "dump-leaf", &dumpLeaf, "dump out histories in each leaf" },
    { OPT_UINT, "internal-order", &internalOrder, "internal order for feature ngram rflm" },
    { OPT_STRING, "ngram-dump", &ngramDump, "compute ngram probs line by line" },
    { OPT_STRING, "info-dump", &infoDump, "output detailed info on ngram computation" }
};

/*
 * Rescore N-best list
 */
void
rescoreNbest(LM &lm, const char *inFilename, const char *outFilename)
{
    NBestList nbList(lm.vocab, maxNbest, splitMultiwords);

    File inlist(inFilename, "r");
    if (!nbList.read(inlist)) {
	cerr << "format error in nbest file\n";
	exit(1);
    }

    if (nbList.numHyps() == 0) {
	cerr << "warning: " << inFilename << " is empty, not rescored\n";
	return;
    }

    if (decipherLM) {
	/*
	 * decipherNoBackoff prevents the Decipher LM from simulating
	 * backoff paths when they score higher than direct probabilities.
	 */
	DecipherNgram oldLM(lm.vocab, decipherOrder, !decipherNoBackoff);

	oldLM.debugme(debug);

	File file(decipherLM, "r");

	if (!oldLM.read(file, limitVocab)) {
	    cerr << "format error in Decipher LM\n";
	    exit(1);
	}

	nbList.decipherFix(oldLM, decipherLMW, decipherWTW);
    }

    nbList.rescoreHyps(lm, rescoreLMW, rescoreWTW);

    if (!noReorder) {
	nbList.sortHyps();
    }

    if (outFilename) {
	File sout(outFilename, "w");
	nbList.write(sout, writeDecipherNbest);
    } else {
	File sout(stdout);
	nbList.write(sout, writeDecipherNbest);
    }
}

LM *
makeMixLM(const char *filename, Vocab &vocab, SubVocab *classVocab,
		    unsigned order, LM *oldLM, double lambda1, double lambda2)
{
    File file(filename, "r");

    /*
     * create factored LM if -factored was specified, 
     * class-ngram if -classes were specified,
     * and otherwise a regular ngram
     */
    Ngram *lm = factored ?
		  new ProductNgram((ProductVocab &)vocab, order) :
    		  (classVocab != 0) ?
		    (simpleClasses ?
			new SimpleClassNgram(vocab, *classVocab, order) :
		        new ClassNgram(vocab, *classVocab, order)) :
		    new Ngram(vocab, order);
    assert(lm != 0);

    lm->debugme(debug);
    lm->skipOOVs() = skipOOVs;
    
    if (!lm->read(file, limitVocab)) {
	cerr << "format error in mix-lm file " << filename << endl;
	exit(1);
    }

    /*
     * Each class LM needs to read the class definitions
     */
    if (classesFile != 0) {
	File file(classesFile, "r");
	((ClassNgram *)lm)->readClasses(file);
    }

    /*
     * Compute mixture lambda (make sure 0/0 = 0)
     */
    Prob lambda = (lambda1 == 0.0) ? 0.0 : lambda1/lambda2;

    if (oldLM == 0) {
	return lm;
    } else if (bayesLength < 0) {
	/*
	 * static mixture
	 */
	((Ngram *)oldLM)->mixProbs(*lm, 1.0 - lambda);
	delete lm;

	return oldLM;
    } else {
	/*
	 * dymamic Bayesian mixture
	 */
	LM *newLM = new BayesMix(vocab, *lm, *oldLM,
				 bayesLength, lambda, bayesScale);
	assert(newLM != 0);

	newLM->debugme(debug);

	return newLM;
    }
}

void
dumpProb(LM &lm, File &fileIn, File &fileOut)
{
  char *line;
  unsigned howmany;
  VocabString words[maxNgramOrder + 1];
  VocabIndex wids[maxNgramOrder + 1];
  LogP prob;
  NgramCount count;

  while (howmany = NgramStats::readNgram(fileIn, words, 
					 maxNgramOrder + 1, count)) {
    if (howmany > order) { continue; }
					   
    lm.vocab.getIndices(words, wids, maxNgramOrder, lm.vocab.unkIndex());

    /*
     * Compute this ngram prob and write it out
     */
    Vocab::reverse(wids);
    prob = lm.wordProb(wids[0], &wids[1]);

    fileOut.fprintf("%e\n", LogPtoProb(prob));
  }
}


int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    /* set default seed for randomization */
    seed = time(NULL) + getpid() + (getppid()<<16);

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (internalOrder == 0) internalOrder = order;

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    if (hmm + adaptMix + decipherHack + tagged +
	skipNgram + hiddenS + df + factored + (hiddenVocabFile != 0) +
	(classesFile != 0 || expandClasses >= 0) + (stopWordFile != 0) > 1)
    {
	cerr << "HMM, AdaptiveMix, Decipher, tagged, factored, DF, hidden N-gram, hidden-S, class N-gram, skip N-gram and stop-word N-gram models are mutually exclusive\n";
	exit(2);
    }

    /*
     * Set random seed
     */
    srand48((long)seed);

    /*
     * Construct language model
     */

    Vocab *vocab;
    Ngram *ngramLM;
    LM *useLM;
    NgramDecisionTree<NgramCount> *dt = 0;
    Discount **discounts;
    NgramStats *intStats;

    if (factored + tagged + multiwords > 1) {
	cerr << "factored, tagged, and multiword vocabularies are mutually exclusive\n";
	exit(2);
    }

    vocab = tagged ? new TaggedVocab :
		multiwords ? new MultiwordVocab :
		      factored ? new ProductVocab :
   			          new Vocab;
    assert(vocab != 0);

    vocab->unkIsWord() = keepunk ? true : false;
    vocab->toLower() = toLower ? true : false;

    if (factored) {
	((ProductVocab *)vocab)->nullIsWord() = keepnull ? true : false;
    }

    /*
     * Change unknown word string if requested
     */
    if (mapUnknown) {
	vocab->remove(vocab->unkIndex());
	vocab->unkIndex() = vocab->addWord(mapUnknown);
    }

    if (vocabFile) {
	File file(vocabFile, "r");
	vocab->read(file);
    }

    if (noneventFile) {
	/*
	 * create temporary sub-vocabulary for non-event words
	 */
	SubVocab nonEvents(*vocab);

	File file(noneventFile, "r");
	nonEvents.read(file);

	vocab->addNonEvents(nonEvents);
    }

    SubVocab *stopWords = 0;
    if (stopWordFile != 0) {
	stopWords = new SubVocab(*vocab);
	assert(stopWords);

	File file(stopWordFile, "r");
	stopWords->read(file);
    }

    SubVocab *hiddenEvents = 0;
    if (hiddenVocabFile != 0) {
	hiddenEvents = new SubVocab(*vocab);
	assert(hiddenEvents);

	File file(hiddenVocabFile, "r");
	hiddenEvents->read(file);
    }

    SubVocab *classVocab = 0;
    if (classesFile != 0 || expandClasses >= 0) {
	classVocab = new SubVocab(*vocab);
	assert(classVocab);

	/*
	 * limitVocab on class N-grams only works if the classes are 
	 * in the vocabulary at read time.  We ensure this by reading 
	 * the class names (the first column of the class definitions)
	 * into the vocabulary.
	 */
	if (limitVocab) {
	    File file(classesFile, "r");
	    classVocab->read(file);
	}
    }

    ngramLM =
       decipherHack ? new DecipherNgram(*vocab, order, !decipherNoBackoff) :
	 df ? new DFNgram(*vocab, order) :
	   skipNgram ? new SkipNgram(*vocab, order) :
	     hiddenS ? new HiddenSNgram(*vocab, order) :
	       tagged ? new TaggedNgram(*(TaggedVocab *)vocab, order) :
	        factored ? new ProductNgram(*(ProductVocab *)vocab, order) :
		 (stopWordFile != 0) ? new StopNgram(*vocab, *stopWords, order):
		   (hiddenVocabFile != 0) ? new HiddenNgram(*vocab, *hiddenEvents, order, hiddenNot) :
		     (classVocab != 0) ? 
			(simpleClasses ?
			    new SimpleClassNgram(*vocab, *classVocab, order) :
			    new ClassNgram(*vocab, *classVocab, order)) :
		        new Ngram(*vocab, order);
    assert(ngramLM != 0);

    ngramLM->debugme(debug);

    if (skipOOVs) {
	ngramLM->skipOOVs() = true;
    }

    if (null) {
	useLM = new NullLM(*vocab);
	assert(useLM != 0);
    } else if (lmFile) {
	if (hmm) {
	    /*
	     * Read an HMM of Ngrams
	     */
	    File file(lmFile, "r");
	    HMMofNgrams *hmm = new HMMofNgrams(*vocab, order);

	    hmm->debugme(debug);

	    if (!hmm->read(file, limitVocab)) {
		cerr << "format error in lm file\n";
		exit(1);
	    }

	    useLM = hmm;
	} else if (adaptMix) {
	    /*
	     * Read an adaptive mixture of Ngrams
	     */
	    File file(lmFile, "r");
	    AdaptiveMix *lm = new AdaptiveMix(*vocab, adaptDecay,
							bayesScale, adaptIters);

	    lm->debugme(debug);

	    if (!lm->read(file, limitVocab)) {
		cerr << "format error in lm file\n";
		exit(1);
	    }

	    useLM = lm;
	} else {
	    /*
	     * Read just a single LM
	     */
	    File file(lmFile, "r");

	    if (!ngramLM->read(file, limitVocab)) {
		cerr << "format error in lm file\n";
		exit(1);
	    }

	    if (mixFile && bayesLength < 0) {
		/*
		 * perform static interpolation (ngram merging)
		 */
		double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3 -
					mixLambda4 - mixLambda5 - mixLambda6 -
					mixLambda7 - mixLambda8 - mixLambda9;

		ngramLM = (Ngram *)makeMixLM(mixFile, *vocab, classVocab,
					order, ngramLM, mixLambda1,
					mixLambda + mixLambda1);

		if (mixFile2) {
		    ngramLM = (Ngram *)makeMixLM(mixFile2, *vocab, classVocab,
					order, ngramLM, mixLambda2,
					mixLambda + mixLambda1 + mixLambda2);
		}
		if (mixFile3) {
		    ngramLM = (Ngram *)makeMixLM(mixFile3, *vocab, classVocab,
					order, ngramLM, mixLambda3,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3);
		}
		if (mixFile4) {
		    ngramLM = (Ngram *)makeMixLM(mixFile4, *vocab, classVocab,
					order, ngramLM, mixLambda4,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4);
		}
		if (mixFile5) {
		    ngramLM = (Ngram *)makeMixLM(mixFile5, *vocab, classVocab,
					order, ngramLM, mixLambda5,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5);
		}
		if (mixFile6) {
		    ngramLM = (Ngram *)makeMixLM(mixFile6, *vocab, classVocab,
					order, ngramLM, mixLambda6,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6);
		}
		if (mixFile7) {
		    ngramLM = (Ngram *)makeMixLM(mixFile7, *vocab, classVocab,
					order, ngramLM, mixLambda7,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6 + mixLambda7);
		}
		if (mixFile8) {
		    ngramLM = (Ngram *)makeMixLM(mixFile8, *vocab, classVocab,
					order, ngramLM, mixLambda8,
					mixLambda + mixLambda1 + mixLambda2 +
					mixLambda3 + mixLambda4 + mixLambda5 +
					mixLambda6 + mixLambda7 + mixLambda8);
		}
		if (mixFile9) {
		    ngramLM = (Ngram *)makeMixLM(mixFile9, *vocab, classVocab,
					order, ngramLM, mixLambda9, 1.0);
		}
	    }

	    /*
	     * Renormalize before the optional steps below, in case input
	     * model needs it, and because class expansion and pruning already
	     * include normalization.
	     */
	    if (renormalize) {
		ngramLM->recomputeBOWs();
	    }

	    /*
	     * Read class definitions from command line AFTER the LM, so
	     * they can override embedded class definitions.
	     */
	    if (classesFile != 0) {
		File file(classesFile, "r");
		((ClassNgram *)ngramLM)->readClasses(file);
	    }

	    if (expandClasses >= 0) {
		/*
		 * Replace class ngram with equivalent word ngram
		 * expandClasses == 0 generates all ngrams
		 * expandClasses > 0 generates only ngrams up to given length
		 */
		Ngram *newLM =
		    ((ClassNgram *)ngramLM)->expand(expandClasses, expandExact);

		newLM->debugme(debug);
		delete ngramLM;
		ngramLM = newLM;
	    }

	    if (prune != 0.0) {
		ngramLM->pruneProbs(prune, minprune);
	    }

	    if (pruneLowProbs) {
		ngramLM->pruneLowProbs(minprune);
	    }

	    useLM = ngramLM;

	    if (dtFile) {

	      /* Read in counts and discounts for dtlm */
	      intStats = new NgramStats(*vocab, order);
	      intStats->openVocab = false;

	      // Read in counts
	      if (trainCountsFile) {
		File file(trainCountsFile, "r");

		if (readWithMincounts) {
		  NgramCount minCounts[order];

		  /* construct min-counts array from -gtNmin options */
		  unsigned i;
		  for (i = 0; i < order && i < maxorder; i ++) {
		    minCounts[i] = gtmin[i + 1];
		  }
		  for ( ; i < order; i ++) {
		    minCounts[i] = gtmin[0];
		  }
		  intStats->readMinCounts(file, order, minCounts);
		} else {
		  intStats->read(file);
		}
	      } else if (trainTextFile) {

		File file(trainTextFile, "r");
		intStats->countFile(file);
	      } else {
		cerr << "need training counts or text file for decision tree language model.\n";
		exit(1);
	      }

	      // Read and merge in heldout counts or text
	      if (heldoutCount) {
		File file(heldoutCount, "r");

		if (readWithMincounts) {
		  NgramCount minCounts[order];

		  /* construct min-counts array from -gtNmin options */
		  unsigned i;
		  for (i = 0; i < order && i < maxorder; i ++) {
		    minCounts[i] = gtmin[i + 1];
		  }
		  for ( ; i < order; i ++) {
		    minCounts[i] = gtmin[0];
		  }
		  intStats->readMinCounts(file, order, minCounts);
		} else {
		  intStats->read(file);
		}
	      } else if (heldoutFile) {
		File file(heldoutFile, "r");
		intStats->countFile(file);
	      }

	      // Read in discounts
	      discounts = new Discount *[order]; assert(discounts);
	      for (int i=0; i<order; i++) {
		if (i >= internalOrder) {
		  discounts[i] = 0;
		  continue;
		}

		if (ukndiscount)
		  discounts[i] = new KneserNey(gtmin[i+1]);
		else 
		  discounts[i] = new ModKneserNey(gtmin[i+1]);

		assert(discounts[i]);
		discounts[i]->debugme(debug);

		if (knFile[i+1] == 0) {
		  cerr << "need kn discount file for order : " << i+1 << endl;
		  exit(1);
		}

		File file(knFile[i+1], "r");
		if (!discounts[i]->read(file)) {
		  cerr << "error in reading discount parameter file "
		       << knFile[i+1] << endl;
		  exit(1);
                }
	      }
	
	      File *dumpfile = 0; // For dumping histories in each leaf
	      File *infodumpfile = 0; /* For dumping detailed info 
					 for ngram computation */

	      if (dtFile) {
		if (dtCounts) {
		  if (dtTest == 0) {
		    cerr << "need dt-test for intermediate test counts\n";
		    exit(1);
		  }
		  
		  MultipassNgramDT<NgramCount> *mdt;
		  mdt = new MultipassNgramDT<NgramCount>(intStats);
		  assert(mdt);

		  mdt->debugme(debug);

		  mdt->setBackoffLM(*ngramLM);
		  
		  // Prepare for estimate() method call
		  mdt->setTreeFileName(dtFile);
		  mdt->setCountFileName(dtCounts);
		  mdt->setTestFileName(dtTest);

		  if (dumpLeaf) {
		    dumpfile = new File(dumpLeaf,"w");
		    mdt->setDumpFile(*dumpfile);
		  }

		  if (infoDump) {
		    infodumpfile = new File(infoDump, "w");
		    mdt->setInfoDumpFile(*infodumpfile);
		  }

		  if (internalOrder > 0) 
		    mdt->setInternalOrder(internalOrder);

		  MultipassNgramDTLM *mdtLM;
		  mdtLM = new MultipassNgramDTLM(*vocab, *mdt, 
						 *discounts[internalOrder-1]);
		  assert(mdtLM);

		  mdtLM->debugme(debug);

		  dt = mdt;
		  useLM = mdtLM;
		} else {
		  dt = new NgramDecisionTree<NgramCount>(*intStats);
		  assert(dt);

		  File dtIn(dtFile, "r");
		  if (!dt->read(dtIn)) {
		    cerr << "format error in dt file.\n";
		    exit(1);
		  }
		  
		  dt->debugme(debug);
		  dt->loadCounts();
		  dt->setBackoffLM(*ngramLM);
		  
		  NgramDecisionTreeLM *dtLM;
		  dtLM = new NgramDecisionTreeLM(*vocab, *dt, 
						 *discounts[order-1]);
		  assert(dtLM);

		  dtLM->debugme(debug);
		  
		  useLM = dtLM;
		}
	      }
	      
	      if (writeLM) {
		if (pplFile || countFile) {
		  NgramStats *testStats = new NgramStats(*vocab, order);
		  assert(testStats);
		  testStats->openVocab = false;

		  if (pplFile) {
		    File testIn(pplFile, "r");
		    testStats->countFile(testIn);
		  }

		  if (countFile) {
		    File testIn(countFile, "r");
		    testStats->read(testIn);
		  }

		  if (!((Ngram *)useLM)->estimate(*testStats, discounts)) {
		    cerr << "decision tree LM estimation failed.\n";
		    exit(1);
		  }
#ifdef DEBUG
		  delete testStats;
#endif
		} else if (!((Ngram *)useLM)->estimate(*intStats, discounts)) {
		  cerr << "decision tree LM estimation failed.\n";
		  exit(1);
		}
		
		delete dumpfile;
	      }
	    }
	}
    } else if (mixNum >= 1) {
      
      int i;
      double *params = new double[mixNum];
      for (i=0; i<mixNum; i++) params[i] = 1/(double)mixNum; // default value

      /* read in mix-param file */
      char *line;
      double t, totalParam = 0.0;
      if (mixParam) {
	i = 0;
	File file(mixParam, "r");
	while (line = file.getline()) {
	  if (sscanf(line, "%lf", &t) != 1) {
	    cerr << "format error in mix-param file\n";
	    exit(1);
	  }
	  if (i < mixNum) { params[i] = t; totalParam += t; i++; }
	  else break;
	}

	if (i != mixNum) {
	  cerr << "number of params in mix-param doesn't agree with mix-num\n";
	  exit(1);
	}

	if (fabs(1.0 - totalParam) > Prob_Epsilon) {
	  cerr << "params in mix-param doesn't add up to 1\n";
	  exit(1);
	}
      }
      
      /* read in mix-list file */
      if (!mixList) {
	cerr << "need -mix-list specified\n";
	exit(1);
      }

      char **fileList = new char*[mixNum];
      i = 0;
      File file(mixList, "r");
      while (line = file.getline()) {
	if (i < mixNum) {
	  fileList[i] = new char[strlen(line)+1];
	  sscanf(line, "%s", fileList[i]);
	} else break;
	i++;
      }
      if (i != mixNum) {
	cerr << "number of files in mix-list doesn't agree with mix-num\n";
	exit(1);
      }

      /* mix LMs */
      File file0(fileList[0], "r");
      if (!ngramLM->read(file0)) {
	cerr << "format error in mix-lm file\n";
	exit(1);
      }

      useLM = ngramLM;
      double param = params[0];
      for (i=1; i<mixNum; i++) {
	param += params[i];
	useLM = makeMixLM(fileList[i], *vocab, classVocab, order, useLM,
			  params[i], param);
      }
      
      for (i=0; i<mixNum; i++) delete[] fileList[i];
      delete[] fileList;
      delete[] params;

    } else {
	cerr << "need at least an -lm file specified\n";
	exit(1);
    }

    if (mixFile && bayesLength >= 0) {
	/*
	 * create a Bayes mixture LM 
	 */
	double mixLambda1 = 1.0 - mixLambda - mixLambda2 - mixLambda3 -
				mixLambda4 - mixLambda5 - mixLambda6 -
				mixLambda7 - mixLambda8 - mixLambda9;

	useLM = makeMixLM(mixFile, *vocab, classVocab, order, useLM,
				mixLambda1,
				mixLambda + mixLambda1);

	if (mixFile2) {
	    useLM = makeMixLM(mixFile2, *vocab, classVocab, order, useLM,
				mixLambda2,
				mixLambda + mixLambda1 + mixLambda2);
	}
	if (mixFile3) {
	    useLM = makeMixLM(mixFile3, *vocab, classVocab, order, useLM,
				mixLambda3,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3);
	}
	if (mixFile4) {
	    useLM = makeMixLM(mixFile4, *vocab, classVocab, order, useLM,
				mixLambda4,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4);
	}
	if (mixFile5) {
	    useLM = makeMixLM(mixFile5, *vocab, classVocab, order, useLM,
				mixLambda5,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5);
	}
	if (mixFile6) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda6,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6);
	}
	if (mixFile7) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda7,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7);
	}
	if (mixFile8) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda8,
				mixLambda + mixLambda1 + mixLambda2 +
				mixLambda3 + mixLambda4 + mixLambda5 +
				mixLambda6 + mixLambda7 + mixLambda8);
	}
	if (mixFile9) {
	    useLM = makeMixLM(mixFile6, *vocab, classVocab, order, useLM,
				mixLambda9, 1.0);
	}
    }

    if (cache > 0) {
	/*
	 * Create a mixture model with the cache lm as the second component
	 */
	CacheLM *cacheLM = new CacheLM(*vocab, cache);
	assert(cacheLM != 0);

	BayesMix *mixLM = new BayesMix(*vocab, *useLM, *cacheLM,
						0, 1.0 - cacheLambda, 0.0);
	assert(mixLM != 0);

        useLM = mixLM;
	useLM->debugme(debug);
    }

    if (dynamic) {
	/*
	 * Create a mixture model with the dynamic lm as the second component
	 */
	DynamicLM *dynamicLM = new DynamicLM(*vocab);
	assert(dynamicLM != 0);

	BayesMix *mixLM = new BayesMix(*vocab, *useLM, *dynamicLM,
						0, 1.0 - dynamicLambda, 0.0);
	assert(mixLM != 0);

        useLM = mixLM;
	useLM->debugme(debug);
    }

    if (adaptMarginals != 0) {
	/* 
	 * Adapt base LM to adaptive marginals given by unigram LM
	 */
	Ngram *adaptMargLM = new Ngram(*vocab, 1);
	assert(adaptMargLM != 0);

	{
	    File file(adaptMarginals, "r");
	    adaptMargLM->debugme(debug);
	    adaptMargLM->read(file);
	}

	LM *baseMargLM;

	if (baseMarginals == 0) {
	    baseMargLM = useLM;
	} else {
	    baseMargLM = new Ngram(*vocab, 1);
	    assert(baseMargLM != 0);

	    File file(baseMarginals, "r");
	    baseMargLM->debugme(debug);
	    baseMargLM->read(file);
	}

	AdaptiveMarginals *adaptLM =
		new AdaptiveMarginals(*vocab, *useLM, *baseMargLM,
					*adaptMargLM, adaptMarginalsBeta);
	if (adaptMarginalsRatios) {
	    adaptLM->computeRatios = true;
	}
	assert(adaptLM != 0);
	useLM = adaptLM;
	useLM->debugme(debug);
    }

    /*
     * Reverse words in scoring
     */
    if (reverse1) {
	useLM->reverseWords = true;
    }

    /*
     * Skip noise tags in scoring
     */
    if (noiseVocabFile) {
	File file(noiseVocabFile, "r");
	useLM->noiseVocab.read(file);
    }
    if (noiseTag) {				/* backward compatibility */
	useLM->noiseVocab.addWord(noiseTag);
    }

    if (memuse) {
	MemStats memuse;
	useLM->memStats(memuse);
	memuse.print();
    }

    /*
     * Apply multiword wrapper if requested
     */
    if (multiwords) {
	useLM = new MultiwordLM(*(MultiwordVocab *)vocab, *useLM);
	assert(useLM != 0);

	useLM->debugme(debug);
    }

    /*
     * Rescore N-gram probs in LM file
     */
    if (rescoreNgramFile) {
	// create new vocab to avoid including class and multiwords
	// from the rescoring LM
	SubVocab *ngramVocab = new SubVocab(*vocab);
	assert(ngramVocab != 0);

	// read N-gram to be rescored
	Ngram *rescoreLM = new Ngram(*ngramVocab, order);
	assert(rescoreLM != 0);
	rescoreLM->debugme(debug);

	File file(rescoreNgramFile, "r");
	rescoreLM->read(file);

	rescoreLM->rescoreProbs(*useLM);

	// free memory for LMs used in rescoring
	if (ngramLM != useLM) {
	    delete ngramLM;
	    ngramLM = 0;
	}
	delete useLM;

	// use rescored LM below
	useLM = rescoreLM;
    }

    /*
     * Compute perplexity on a text file, if requested
     */
    if (pplFile) {
	File file(pplFile, "r");
	TextStats stats;

	/*
	 * Send perplexity info to stdout 
	 */
	useLM->dout(cout);
	useLM->pplFile(file, stats, escape);
	useLM->dout(cerr);

	cout << "file " << pplFile << ": " << stats;
    }

    /*
     * Compute perplexity on a count file, if requested
     */
    if (countFile) {
      /*
       * Compute ngram probs in a count file line by line
       */
      if (ngramDump) {
      
	File fileIn(countFile, "r");
	File fileOut(ngramDump, "w");
	dumpProb(*useLM, fileIn, fileOut);
      
      } else {

	TextStats stats;
	File file(countFile, "r");

	/*
	 * Send perplexity info to stdout 
	 */
	useLM->dout(cout);
	useLM->pplCountsFile(file, countOrder ? countOrder : order,
						stats, escape, countEntropy);
	useLM->dout(cerr);

	cout << "file " << countFile << ": " << stats;
      }
    }

    /*
     * Rescore N-best list, if requested
     */
    if (nbestFile) {
	rescoreNbest(*useLM, nbestFile, NULL);
    }

    /*
     * Rescore multiple N-best lists
     */
    if (nbestFiles) {
	File file(nbestFiles, "r");

	char *line;
	while (line = file.getline()) {
	    char *fname = strtok(line, wordSeparators);
	    if (!fname) continue;

	    RefString sentid = idFromFilename(fname);

	    if (writeNbestDir) {
		char scoreFile[strlen(writeNbestDir) + 1
				 + strlen(sentid) + strlen(GZIP_SUFFIX) + 1];
		sprintf(scoreFile, "%s/%s%s", writeNbestDir, sentid,
								GZIP_SUFFIX);
		rescoreNbest(*useLM, fname, scoreFile);
	    } else {
		rescoreNbest(*useLM, fname, NULL);
	    }
	}
    }

    /*
     * Rescore stream of N-best hyps, if requested
     */
    if (rescoreFile) {
	File file(rescoreFile, "r");

	LM *oldLM;

	if (decipherLM) {
	    oldLM =
		 new DecipherNgram(*vocab, decipherOrder, !decipherNoBackoff);
	    assert(oldLM != 0);

	    oldLM->debugme(debug);

	    File file(decipherLM, "r");

	    if (!oldLM->read(file, limitVocab)) {
		cerr << "format error in Decipher LM\n";
		exit(1);
	    }
	} else {
	    /*
	     * Create dummy LM for the sake of rescoreFile()
	     */
	    oldLM = new NullLM(*vocab);
	    assert(oldLM != 0);
	}

	useLM->rescoreFile(file, rescoreLMW, rescoreWTW,
				*oldLM, decipherLMW, decipherWTW, escape);

#ifdef DEBUG
	delete oldLM;
#endif
    }

    if (generate1) {
	File outFile(stdout);
	unsigned i;

	for (i = 0; i < generate1; i++) {
	    VocabString *sent = useLM->generateSentence(maxWordsPerLine,
							(VocabString *)0);
	    Vocab::write(outFile, sent);
	    putchar('\n');
	}
    }

    if (writeLM) {
	File file(writeLM, "w");
	useLM->write(file);
    }

    if (writeVocab) {
	File file(writeVocab, "w");
	vocab->write(file);
    }

    if (runServer) {
      ServerSocket socket(serverPort);
      LMServer server(*useLM, socket);
      server.debugme(debug);
      
      server.run();
    }

#ifdef DEBUG
    if (&ngramLM->vocab != vocab) {
	delete &ngramLM->vocab;
    }
    if (ngramLM != useLM) {
	delete ngramLM;
    }
    delete useLM;

    delete dt;
    for (int i=0; i<order; i++) delete discounts[i];
    delete[] discounts;
    delete intStats;

    delete stopWords;
    delete hiddenEvents;
    delete classVocab;
    delete vocab;

    return 0;
#endif /* DEBUG */

    exit(0);
}

