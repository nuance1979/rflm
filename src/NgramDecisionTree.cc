/*
 * NgramDecisionTree.cc --
 *      Decision tree for Ngram LM 
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include <time.h>

#include "NgramDecisionTree.h"
#include "Array.h"

#include "NgramStats.cc"
#include "DecisionTree.cc"
#include "Array.cc"

#define DEBUG_PRUNE_TREE 3
#define DEBUG_NGRAM_PROB 4
#define DEBUG_SPLIT_NODE 5
#define DEBUG_EXCHANGE  2

#define NODE(p) ((NgramDecisionTreeNode<CountT> *)(p))
#define ROOT NODE(this->root)
#define QUESTION(q) ((NgramQuestion *)(q))

#define INSTANTIATE_NGRAMDECISIONTREE_DSTRUCT(CountT) \
  template class Element<CountT>; \
  INSTANTIATE_DECISIONTREE(VocabIndex, Element<CountT>); \
  INSTANTIATE_NGRAMCOUNTS(CountT); \
  template class NgramDecisionTreeNode<CountT>; \
  template class NgramDecisionTree<CountT>; \
  INSTANTIATE_ARRAY(NgramDecisionTree<CountT> *)

#ifdef USE_SARRAY

#include "SArray.cc"

#define INSTANTIATE_NGRAMDECISIONTREE(CountT)	\
  INSTANTIATE_SARRAY(VocabIndex, CountT);	\
  INSTANTIATE_NGRAMDECISIONTREE_DSTRUCT(CountT)

#else /* ! USE_SARRAY */

#include "LHash.cc"

#define INSTANTIATE_NGRAMDECISIONTREE(CountT)	\
  INSTANTIATE_NGRAMDECISIONTREE_DSTRUCT(CountT)

#endif /* USE_SARRAY */

template <class CountT>
inline double
NgramDecisionTree<CountT>::timesLog(CountT count)
{
  if (count > 0) return count*log(count);
  else return 0.0; // 0*log(0) = 0
}

template <class CountT>
unsigned
NgramDecisionTree<CountT>::cloneCounts(NgramCounts<CountT> &dest,
				       NgramCounts<CountT> &source)
{
  // Should have been a clone method in class NgramCounts<CountT>
  assert(dest.getorder() == source.getorder());

  unsigned int t = 0;
  unsigned int order = source.getorder();
  VocabIndex ngram[order+1];
  CountT *count;
  NgramCountsIter<CountT> iter(source, ngram, order);
  while (count = iter.next()) {
    *(dest.insertCount(ngram)) = *count;
    t++;
  }

  return t;
}

template <class CountT>
void
NgramDecisionTree<CountT>::printSet(const SetType &set)
{
  VocabIndex word;
  SetTypeIter iter(set);
  dout() << set.numEntries() << ":";
  while (iter.next(word)) 
    dout() << " " << vocab.getWord(word);
  dout() << endl;
}

template <class CountT>
void
NgramDecisionTree<CountT>::printElement(const ElementType &element)
{
  dout() << "sum: " << element.sum << ", ";
  CountT *count;
  VocabIndex word;
  FutureCountsIter iter(element.future);
  dout() << "future: " << element.future.numEntries() << ":";
  while (count = iter.next(word)) 
    dout() << " " << vocab.getWord(word) << "(" << *count << ")";
  dout() << endl;
}

template <class CountT>
void
NgramDecisionTree<CountT>::printStats()
{
  TreeStats s;
  treeStats(s, this->root);
  dout() << "numNodes: " << s.numNodes
	 << ", numLeaves: " << s.numLeaves
	 << ", depth: " << s.depth
	 << endl;
}

template <class CountT>
NgramDecisionTree<CountT>::NgramDecisionTree(NgramCounts<CountT> &cnts,
					     RandType randType, double r)
  : cnts(cnts), LMStats(cnts.vocab), order(cnts.getorder()),
    randType(randType), _r(r), dumpfile(0), infoDumpFile(0),
    alpha(0)
{

}

template <class CountT>
NgramDecisionTree<CountT>::~NgramDecisionTree()
{
}

template <class CountT>
unsigned int
NgramDecisionTree<CountT>::countSentence(const VocabString *words)
{
  return 0;
}

template <class CountT>
unsigned int
NgramDecisionTree<CountT>::countSentence(const VocabIndex *words)
{
  return 0;
}

template <class CountT>
unsigned int
NgramDecisionTree<CountT>::countSentence(const VocabString *words, 
					 const char *weight)
{
  return 0;
}

template <class CountT>
void
NgramDecisionTree<CountT>::memStats(MemStats &stats)
{
}

template <class CountT>
Boolean
NgramDecisionTree<CountT>::read(File &file)
{
  Boolean ok;
  //  ROOT = readTree(file, ok);
  this->root = readTree(file,ok);
  return ok;
}

template <class CountT>
NgramDecisionTreeNode<CountT> *
NgramDecisionTree<CountT>::readTree(File &file, Boolean &ok)
{
  char *line = file.getline();
  if (line == 0) { ok = true; return 0; }

  NgramDecisionTreeNode<CountT> *p = 0;
  if (strcmp(line, "node\n") == 0) {
    p = new NgramDecisionTreeNode<CountT>(); assert(p);
    p->question = new NgramQuestion(); assert(p->question);
    ok = QUESTION(p->question)->readText(file, vocab);
    if (ok) {
      //      NODE(p->left) = readTree(file, ok); 
      p->left = readTree(file, ok);
      //      if (ok) NODE(p->right) = readTree(file, ok); 
      if (ok) p->right = readTree(file, ok);
    }
  } else if (strcmp(line, "leaf\n") == 0) {
    p = new NgramDecisionTreeNode<CountT>(); assert(p);
    p->question = new NgramQuestion(); assert(p->question);
    ok = QUESTION(p->question)->readText(file, vocab);
  } else if (strcmp(line, "null\n") == 0) {
    ok = true;
  } else
    ok = false;

  if (ok) return p;
  else { delete p; return 0; }
}

template <class CountT>
void
NgramDecisionTree<CountT>::write(File &file)
{
  writeTree(file, ROOT);
}

template <class CountT>
void
NgramDecisionTree<CountT>::writeTree(File &file, 
				     NgramDecisionTreeNode<CountT> *p)
{
  if (p == 0) { 
    fprintf(file, "null\n"); 
    return; 
  }

  if (p->isLeaf()) {
    fprintf(file, "leaf\n");
    assert(p->question != 0);
    QUESTION(p->question)->writeText(file, vocab);
  } else {
    fprintf(file, "node\n");
    assert(p->question != 0);
    QUESTION(p->question)->writeText(file, vocab);
    writeTree(file, NODE(p->left));
    writeTree(file, NODE(p->right));
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::grow(unsigned seed)
{
  // Initialize
  if (seed == 0) seed = time(0);
  dout() << "seed= " << seed << endl;
  srand(seed);
  this->root = new NgramDecisionTreeNode<CountT>();
  assert(this->root != 0);
  ROOT->question = new NgramQuestion();
  ROOT->counts = new NgramCounts<CountT>(vocab, order);
  cloneCounts(*(ROOT->counts), cnts);

  Array<NgramDecisionTreeNode<CountT> *> leaves;
  int current = 0, offset;
  leaves[0] = ROOT;
  
  // Split the node
  double gain;
  NgramDecisionTreeNode<CountT> *left, *right, *node;
  while (current < leaves.size()) {
    node = leaves[current]; assert(node);
    left = new NgramDecisionTreeNode<CountT>();
    right = new NgramDecisionTreeNode<CountT>();
    if (splitNode(*node, left, right, gain)) {
      node->left = left; node->right = right;
      delete node->counts; // Internal node doesn't need 
                          // to keep counts
      offset = leaves.size();
      leaves[offset] = left;  leaves[offset + 1] = right;
    } else {
      delete left; delete right;
    }
    current++;
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::splitNodeRandomizer(unsigned int order,
                                               unsigned int *posSet,
                                               unsigned int &posSetSize)
{
  assert(posSet);
  unsigned int i;

  if (randType == NO_RAND || randType == RAND_INIT) {
    for (i=0; i<order-1; i++) posSet[i] = i; 
    posSetSize = order - 1;
  } else if (alpha != 0) {
    posSetSize = 0;
    while (posSetSize <= 0) {
      for (i=0; i<order-1; i++)
	if (rand() < RAND_MAX*alpha[i+1]) {
	  posSet[posSetSize] = i;
	  posSetSize++;
	}
    }
  } else {
    posSetSize = 0;
    while (posSetSize <= 0) {
      for (i=0; i<order-1; i++)
	if (rand() < RAND_MAX*_r) {
	  posSet[posSetSize] = i;
	  posSetSize++;
	}
    }
  }
}

template <class CountT>
Boolean
NgramDecisionTree<CountT>::splitNode(const NgramDecisionTreeNode<CountT> &node,
				     NgramDecisionTreeNode<CountT> *left,
				     NgramDecisionTreeNode<CountT> *right,
				     double &increase)
{
  unsigned int ind, pos, bestPos;
  unsigned moved1, moved2;
  ELEM_INDEX_T<VocabIndex, ElementType> elements;
  SetType setA, setAbar, bestA, bestAbar;
  ElementType countsA, countsAbar;
  double gain;
  unsigned int posSet[order];
  unsigned int posSetSize;
  /* increase = -99; 
     for (int i=0; i<100; i++) { */
  increase = -99;
  splitNodeRandomizer(order, posSet, posSetSize);

  if (debug(DEBUG_EXCHANGE)) {
    dout() << "SplitNode: start for " << posSetSize << " positions\n";
  }
  
  for (ind=0; ind<posSetSize; ind++) {
    pos = posSet[ind];

    buildElements(node, pos, elements);
    
    if (debug(DEBUG_EXCHANGE)) {
      dout() << "Position: " << pos 
	     << ", #elements: " << elements.numEntries() << endl;
    }

    if (elements.numEntries() >=2) {
      
      setA.clear(); setAbar.clear();
      countsA.clear(); countsAbar.clear();
      initSplit(elements, setA, countsA, setAbar, countsAbar);
      
      if (debug(DEBUG_EXCHANGE)) {
	dout() << "SplitNode: init " << setA.numEntries() << "+"
	       << setAbar.numEntries() << " for position " << pos 
	       << endl;
      }
      
      if (debug(DEBUG_SPLIT_NODE)) {
	dout() << "setA    = "  
	       << setA.numEntries() << endl; //printSet(setA);
	dout() << "setAbar = " 
	       << setAbar.numEntries() << endl; //printSet(setAbar);
	dout() << "countsA    = " 
	       << countsA.sum << endl; //printElement(countsA);
	dout() << "countsAbar = "
	       << countsAbar.sum << endl; //printElement(countsAbar);
      }
    
      // Initialize the gain
      double scoreNode;
      gain = scoreSplit(elements, setA, setAbar, &scoreNode);
      gain -= scoreNode;
    
      do {
	moved1 = moveElements(elements,setA,countsA,setAbar,countsAbar,gain);
	moved2 = moveElements(elements,setAbar,countsAbar,setA,countsA,gain);
	
	if (debug(DEBUG_EXCHANGE)) {
	  dout() << "Exchange: " << moved1+moved2 << " items out of "
		 << setA.numEntries() << "+" << setAbar.numEntries()
		 << " exchanged.\n";
	}
      } while (moved1 + moved2 > 0);
    
      if (debug(DEBUG_SPLIT_NODE)) {
	dout() << "gain: " << gain << endl;
      }
      
      if (gain > increase) {
	// Copy setA and setAbar
	VocabIndex wd;
	SetTypeIter iterA(setA), iterAbar(setAbar);
	
	bestA.clear();
	while (iterA.next(wd)) bestA.insert(wd);
	bestAbar.clear();
	while (iterAbar.next(wd)) bestAbar.insert(wd);
	
	bestPos = pos;
	increase = gain;
      }
    }

    // Release memory!!!
    ELEM_ITER_T<VocabIndex, ElementType> eIter(elements);
    ElementType *data;
    VocabIndex word;
    while (data = eIter.next(word)) data->future.clear();
    elements.clear();
  }

  if (debug(DEBUG_SPLIT_NODE)) {
    dout() << "bestPos: " << bestPos 
	   << ", increase: " << increase << endl;
    dout() << "bestA    = "
	   << bestA.numEntries() << endl; //printSet(bestA);
    dout() << "bestAbar = "
	   << bestAbar.numEntries() << endl; //printSet(bestAbar);
  }

  // Stopping criteria
  // NOTE: increase <= -1 in Peng's implementation
  if (increase <= 0 || bestA.numEntries() == 0 || bestAbar.numEntries() == 0) 
    return false;

  if (debug(DEBUG_EXCHANGE)) {
    dout() << "Splitting...\n";
  }

  // Split data
  assert(left != 0);
  NODE(left)->counts = new NgramCounts<CountT>(vocab, order);
  assert(left->counts != 0);
  left->question = new NgramQuestion(bestA, bestPos); 
  assert(left->question != 0);

  assert(right != 0);
  NODE(right)->counts = new NgramCounts<CountT>(vocab, order);
  assert(right->counts != 0);
  right->question = new NgramQuestion(bestAbar, bestPos);
  assert(right->question != 0);

  CountT *count, *data;
  VocabIndex ngram[order+1];
  NgramCountsIter<CountT> iter(*(node.counts), ngram, order);
  while (count = iter.next()) {
    Vocab::reverse(ngram);
    // NOTE: ngram[bestPos+1] is conceptually context[bestPos]!
    if (bestA.find(ngram[bestPos+1])) {
      Vocab::reverse(ngram);
      data = NODE(left)->counts->insertCount(ngram);
      *data = *count;
    } else {
      assert(bestAbar.find(ngram[bestPos+1]) != 0);
      Vocab::reverse(ngram);
      data = NODE(right)->counts->insertCount(ngram);
      *data = *count;
    }
  }

  if (debug(DEBUG_SPLIT_NODE)) {
    dout() << "data split: node = " << node.counts->sumCounts()
	   << ", left = " << NODE(left)->counts->sumCounts()
	   << ", right = " << NODE(right)->counts->sumCounts() << endl;
  }

  return true;
}

template <class CountT>
void
NgramDecisionTree<CountT>::buildElements(
    const NgramDecisionTreeNode<CountT> &node, 
    const unsigned int pos,
    ELEM_INDEX_T<VocabIndex, ElementType> &elements)
{
  assert(node.counts != 0); assert(pos < order-1);

  VocabIndex ngram[order+1];
  ElementType *element;
  CountT *count, *data;
  NgramCountsIter<CountT> iter(*(node.counts), ngram, order);
  while (count = iter.next()) {
    Vocab::reverse(ngram);
    // NOTE: ngram[pos+1] is conceptually context[pos]!
    element = elements.insert(ngram[pos+1]); assert(element != 0);
    data = element->future.insert(ngram[0]);
    *data += *count;
    element->sum += *count;
    Vocab::reverse(ngram);
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::initSplitRandomizer(
    const ELEM_INDEX_T<VocabIndex, ElementType> &elements, 
    SetType &setA, SetType &setAbar)
{
  VocabIndex word; 
  ELEM_ITER_T<VocabIndex, ElementType> iter(elements);

  if (randType == NO_RAND || randType == RAND_SELECT) {
    setA.clear(); setAbar.clear();
    iter.init();
    while (iter.next(word)) {
      if (word % 2 == 0)
	setA.insert(word);
      else
	setAbar.insert(word);
    }
    return ;
  } else {
    while (1) {
      setA.clear(); setAbar.clear();
      iter.init();
      while (iter.next(word)) { 
	if (rand() < RAND_MAX/2) {
	  setA.insert(word);
	} else { 
	  setAbar.insert(word);
	}
      }
      if (setA.numEntries() > 0 && setAbar.numEntries() > 0) return;
    }
  }
}


template <class CountT>
void
NgramDecisionTree<CountT>::initSplit(
    const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
    SetType &setA, ElementType &countsA, 
    SetType &setAbar, ElementType &countsAbar)
{
  ElementType *data;
  VocabIndex word;
  VocabIndex wd;
  CountT *count;
  
  initSplitRandomizer(elements, setA, setAbar);
  ELEM_ITER_T<VocabIndex, ElementType> iter(elements);
  while (data = iter.next(word)) {
    if (setA.find(word)) {
      // Update countsA
      FutureCountsIter cIter(data->future);
      while (count = cIter.next(wd)) {
	*(countsA.future.insert(wd)) += *count;
	countsA.sum += *count;
      }
    } else {
      assert(setAbar.find(word));

      // Update futureAbar
      FutureCountsIter cIter(data->future);
      while (count = cIter.next(wd)) {
	*(countsAbar.future.insert(wd)) += *count;
	countsAbar.sum += *count;
      }
    }
  }
}

template <class CountT>
unsigned
NgramDecisionTree<CountT>::moveElements(
    const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
    SetType &from, ElementType &countsFrom,
    SetType &to, ElementType &countsTo,
    double &accumulator)
{
  unsigned moved = 0;
  ElementType *dataV;
  VocabIndex v, w;
  VocabIter vIter(vocab);
  double delta;
  CountT *x1, *x2, *y, zeroCount = 0;
  SetType from1;

  double before, after;
  if (debug(DEBUG_SPLIT_NODE)) {
    dout() << "from.numEntries: " << from.numEntries() << endl;
    /*   before = scoreSplit(elements, from, to); */
  }

  // Make a copy so that iter works fine
  SetTypeIter iter(from);
  while (iter.next(v)) from1.insert(v);

  SetTypeIter iterV(from1); 
  while (iterV.next(v)) { 
    dataV = elements.find(v); assert(dataV);
    // Compute log-likelihood increase
    delta = 0.0;
    FutureCountsIter iterW(dataV->future);
    while (y = iterW.next(w)) {      
      x1 = countsFrom.future.find(w); assert(x1 != 0);
      delta += timesLog(*x1 - *y) - timesLog(*x1);
      x2 = countsTo.future.find(w); if (!x2) x2 = &zeroCount; 
      delta += timesLog(*x2 + *y) - timesLog(*x2);
    }
    y = &(dataV->sum);
    x1 = &countsFrom.sum;    x2 = &countsTo.sum;
    delta += timesLog(*x1) - timesLog(*x1 - *y);
    delta += timesLog(*x2) - timesLog(*x2 + *y);
    /*
    if (debug(DEBUG_SPLIT_NODE)) {
      dout() << "delta: " << delta 
	     << " for " << vocab.getWord(v) 
	     << endl; 
    }
    */
    // Update A, Abar and their future counts
    if (delta > 1e-5) { // delta > 0 would cause endless loop 
                        // due to float error
      from.remove(v); to.insert(v); moved++;
      iterW.init();
      while (y = iterW.next(w)) {
	x1 = countsFrom.future.find(w); assert(x1);
	*x1 -= *y;
	x2 = countsTo.future.insert(w); assert(x2);
	*x2 += *y;
      }
      countsFrom.sum -= dataV->sum;
      countsTo.sum += dataV->sum;
      /*
      if (debug(DEBUG_SPLIT_NODE)) {
	after = scoreSplit(elements, from, to);
	dout() << "delta: " << (after - before)
	       << " for " << vocab.getWord(v) 
	       << endl;      
      }
      */
      accumulator += delta;
    }
  }
  
  return moved;
}

template <class CountT>
LogP
NgramDecisionTree<CountT>::wordProb(Discount &discount, LM &backoffLM, 
				    VocabIndex word, const VocabIndex *context)
{
  setDiscount(discount); setBackoffLM(backoffLM);
  return wordProb(word, context);
}

template <class CountT>
LogP
NgramDecisionTree<CountT>::wordProb(VocabIndex word, const VocabIndex *context)
{
  // Bail out due to low order
  unsigned len = Vocab::length(context);
  if (len < order-1) {
    seenHistory = false;
    return backoffLM->wordProb(word, context);
  }

  // Get backoff context
  if (order < 2) 
    return wordProb(word, context, backoffLM->wordProb(word, 0));
  else { // order >= 2 
    VocabIndex boContext[order]; 
    for (int i=0; i<order; i++) boContext[i] = context[i]; // len >= order - 1
    boContext[order-2] = Vocab_None;

    return wordProb(word, context, backoffLM->wordProb(word, boContext));
  }
}

template <class CountT>
LogP
NgramDecisionTree<CountT>::wordProb(VocabIndex word, const VocabIndex *context,
				    LogP backoffProb)
{
  Boolean foundP;
  
  if (debug(DEBUG_NGRAM_PROB)) {
    dout() << "Prob( " << vocab.getWord(word);
    if (context && Vocab::length(context) > 0) {
      dout() << " |";
      for (unsigned i=0; i<Vocab::length(context); i++) {
	if (i > order-2) { 
	  dout() << " ..."; 
	  break; 
	} else 
	  dout() << " " << vocab.getWord(context[i]);
      }
    }
    dout() << " ) = ";
  }

  ElementType *data = this->find(context, foundP);

  seenHistory = foundP; // a little redundant here...

  if (foundP)
    return wordProb(data, word, backoffProb);
  else // Bail out due to history classification failure
    return backoffLM->wordProb(word, context);
}

template <class CountT>
LogP
NgramDecisionTree<CountT>::wordProb(ElementType *data, VocabIndex word,
				    LogP backoffProb)
{
  LogP lprob;
  CountT *count, totalCount;
  Count observedVocab, min2Vocab, min3Vocab;
  Prob prob;

  assert(data != 0);
  totalCount = data->sum; assert(totalCount > 0);
  count = data->future.find(word);
  observedVocab = data->future.numEntries();
  min2Vocab = data->min2Vocab;
  min3Vocab = data->min3Vocab;

  double dist, backoffWeight;
  
  if (count) {
    dist = discount->discount(*count, totalCount, observedVocab);
    prob = (dist * *count) / totalCount;
  } else prob = 0.0;
  
  seenHistory = true;
  seenFuture = (count != 0); // again, a little redundant...

  // always interpolate
  backoffWeight = discount->lowerOrderWeight(totalCount, observedVocab, 
					     min2Vocab, min3Vocab); 
  prob += backoffWeight * LogPtoProb(backoffProb);

  if (debug(DEBUG_NGRAM_PROB)) {
    dout() << " backoffWeight: " << backoffWeight;
    dout() << " backoffProb: " << LogPtoProb(backoffProb);
    dout() << " prob: " << prob << endl;
    //    printElement(*data);
  } 

  lprob = ProbToLogP(prob);

  return lprob;
}

template <class CountT>
void
NgramDecisionTree<CountT>::prune(const double threshold)
{
  assert(discount); assert(backoffLM); assert(heldout);
  if (this->root == 0) return;

  ROOT->counts = new NgramCounts<CountT>(vocab, order);
  cloneCounts(*(ROOT->counts), cnts);
  
  ROOT->heldout = new NgramCounts<CountT>(vocab, order);
  cloneCounts(*(ROOT->heldout), *heldout);

  LogP LogL;
  CountT Sum;
  computePotential(ROOT, LogL, Sum);

  // pruning according to threshold
  this->threshold = threshold;
  pruneTree(ROOT);
}

template <class CountT>
void
NgramDecisionTree<CountT>::pruneTree(NgramDecisionTreeNode<CountT> *p)
{
  /* NOTE: This function uses postorder traversal for the sake of 
     debugging with MultipassNgramDT::prune(). For best performance,
     use preorder instead.
  */

  if (p == 0 || p->isLeaf()) return;

  pruneTree(NODE(p->left));
  pruneTree(NODE(p->right));

  if (p->potential <= threshold) {

    /* NOTE: Because thoeretically p->potential can take values 
       as small as Prob_Epsilon , there is NO way to garantee this pruning
       procedure gives exactly the same result as NgramDecisionTree::prune()
       even though it should!!!

       The only way to get around this might be to use double instead of 
       float for lprob. But since SRI LM Toolkit is using float for lprob
       anyway, I decide to settle for float as well.
    */

    freeTree(p->left); p->left = 0;
    freeTree(p->right); p->right = 0;
  }
}				     

template <class CountT>
void
NgramDecisionTree<CountT>::computePotential(NgramDecisionTreeNode<CountT> *p,
					    LogP &leavesLikelihood,
					    CountT &leavesSum)
{
  if (p == 0) { leavesLikelihood = 0.0; leavesSum = 0; return; }
  assert(p->counts != 0); assert(p->heldout != 0);

  VocabIndex ngram[order + 1];
  CountT *count, heldoutSum;
  LogP backoffProb, LogL;

  // Prepare future counts p->data
  p->data = new ElementType(); assert(p->data != 0);
  NgramCountsIter<CountT> iterC(*(p->counts), ngram, order);
  while (count = iterC.next()) {
    Vocab::reverse(ngram);
    *(p->data->future.insert(ngram[0])) += *count; 
    p->data->sum += *count;
    Vocab::reverse(ngram);
  }
  p->data->prepare();

  if (debug(DEBUG_PRUNE_TREE)) {
    dout() << "counts = " 
	   << p->data->sum << endl; // printElement(*(p->data));
  }
  
  // Compute LL for heldout
  VocabIndex tmp; 
  LogL = 0.0; heldoutSum = 0;
  NgramCountsIter<CountT> iterH(*(p->heldout), ngram, order);
  while (count = iterH.next()) {
    Vocab::reverse(ngram);

    // Get backoff context cf. wordProb(VocabIndex, const VocabIndex *)
    tmp = ngram[order-1]; ngram[order-1] = Vocab_None;
    backoffProb = backoffLM->wordProb(ngram[0], &ngram[1]);
    ngram[order-1] = tmp;

    LogL += *count * wordProb(p->data, ngram[0], backoffProb);
    heldoutSum += *count;
    Vocab::reverse(ngram);
  }

  if (debug(DEBUG_PRUNE_TREE)) {
    dout() << "heldoutSum = "
	   << heldoutSum << endl;
  }

  // Pour down data to children
  if (p->isLeaf()) { 
    delete p->counts; delete p->heldout; delete p->data; // save memory
    p->counts = 0; p->heldout = 0; p->data = 0; // so that double free is safe

    p->potential = 0;
    leavesLikelihood = LogL;
    leavesSum = heldoutSum;

    if (debug(DEBUG_PRUNE_TREE)) {
      dout() << "leaf potential: " << p->potential
	     << ", LogL: " << LogL
	     << ", heldoutSum: " << heldoutSum
	     << ", leavesLikelihood: " << leavesLikelihood
	     << ", leavesSum: " << leavesSum
	     << endl;
    }

    return;
  }

  if (p->left) { 
    NODE(p->left)->counts = new NgramCounts<CountT>(vocab, order); 
    assert(NODE(p->left)->counts);
    NODE(p->left)->heldout = new NgramCounts<CountT>(vocab, order); 
    assert(NODE(p->left)->heldout);
  }
  if (p->right) {
    NODE(p->right)->counts = new NgramCounts<CountT>(vocab, order); 
    assert(NODE(p->right)->counts);
    NODE(p->right)->heldout = new NgramCounts<CountT>(vocab, order); 
    assert(NODE(p->right)->heldout);
  }
  iterC.init();
  while (count = iterC.next()) {
    Vocab::reverse(ngram);
    if (p->left) {
      assert(NODE(p->left)->question);
      if (NODE(p->left)->question->isTrue(&ngram[1])) {
	Vocab::reverse(ngram);
	*(NODE(p->left)->counts->insertCount(ngram)) = *count;
	Vocab::reverse(ngram);
      }
    }
    if (p->right) {
      assert(NODE(p->right)->question);
      if (NODE(p->right)->question->isTrue(&ngram[1])) {
	Vocab::reverse(ngram);
	*(NODE(p->right)->counts->insertCount(ngram)) = *count;
	Vocab::reverse(ngram);
      }
    }
    Vocab::reverse(ngram);
  }
  iterH.init();
  while (count = iterH.next()) {
    Vocab::reverse(ngram);
    if (p->left) {
      assert(NODE(p->left)->question);
      if (NODE(p->left)->question->isTrue(&ngram[1])) {
	Vocab::reverse(ngram);
	*(NODE(p->left)->heldout->insertCount(ngram)) = *count;
	Vocab::reverse(ngram);
      }
    }
    if (p->right) {
      assert(NODE(p->right)->question);
      if (NODE(p->right)->question->isTrue(&ngram[1])) {
	Vocab::reverse(ngram);
	*(NODE(p->right)->heldout->insertCount(ngram)) = *count;
	Vocab::reverse(ngram);
      }
    }
    Vocab::reverse(ngram);
  }

  delete p->counts; delete p->heldout; delete p->data; // save memory 
  p->counts = 0; p->heldout = 0; p->data = 0; // so that double free is safe

  LogP leftLogL, rightLogL;
  CountT leftSum, rightSum;
  computePotential(NODE(p->left), leftLogL, leftSum);
  computePotential(NODE(p->right), rightLogL, rightSum);

  // Return statistics
  leavesLikelihood = leftLogL + rightLogL;
  leavesSum = leftSum + rightSum;

  // Compute potential
  p->potential = ((leavesSum==0) ? 0 : leavesLikelihood/leavesSum) 
    - ((heldoutSum==0)? 0 : LogL/heldoutSum);

  if (debug(DEBUG_PRUNE_TREE)) {
    dout() << "node potential: " << p->potential
	   << ", LogL: " << LogL
	   << ", heldoutSum: " << heldoutSum
	   << ", leavesLikelihood: " << leavesLikelihood
	   << ", leavesSum: " << leavesSum
	   << endl;
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::loadCounts(NgramCounts<CountT> &counts)
{
  Boolean foundP;
  ElementType *data;
  CountT *count;
  VocabIndex ngram[order+1];

  clearCounts();
  NgramCountsIter<CountT> iter(counts, ngram, order);
  while (count = iter.next()) {
    Vocab::reverse(ngram);
    data = this->find(&ngram[1], foundP); // NOTE: &ngram[1] is context!
    if (foundP) { 
      assert(data);
      *(data->future.insert(ngram[0])) += *count;
      data->sum += *count;
    }
    Vocab::reverse(ngram); // Important!!!
  }

  // Prepare for Modified Kneser-Ney smoothing
  prepareCounts();
}

template <class CountT>
void
NgramDecisionTree<CountT>::prepareCounts()
{
  prepareCountsTree(ROOT);
}

template <class CountT>
void
NgramDecisionTree<CountT>::prepareCountsTree(NgramDecisionTreeNode<CountT> *p)
{
  if (p == 0) return;
  
  if (p->isLeaf()) {
    if (p->data) p->data->prepare();
  } else {
    prepareCountsTree(NODE(p->left));
    prepareCountsTree(NODE(p->right));
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::clearCounts()
{
  clearCountsTree(ROOT);
}

template <class CountT>
void
NgramDecisionTree<CountT>::clearCountsTree(NgramDecisionTreeNode<CountT> *p)
{
  if (!p) return;
  if (p->isLeaf()) {
    if (p->data) p->data->clear();
  } else {
    clearCountsTree(NODE(p->left));
    clearCountsTree(NODE(p->right));
  }
}

template <class CountT>
void
NgramDecisionTree<CountT>::printCounts()
{
  printCountsTree(ROOT);
}

template <class CountT>
void
NgramDecisionTree<CountT>::printCountsTree(NgramDecisionTreeNode<CountT> *p)
{
  if (!p) return;
  if (p->isLeaf()) {
    assert(p->data);
    printElement(*(p->data));
    if (p->data->sum <= 0) { 
      dout() << "Warning: no data for this leaf\n";
    }
  } else {
    printCountsTree(NODE(p->left));
    printCountsTree(NODE(p->right));
  }
}

template <class CountT>
CountT *
NgramDecisionTree<CountT>::findCount(const VocabIndex *context, 
				     Boolean &foundP)
{
  ElementType *element = this->find(context, foundP);
  if (foundP) {
    assert(element != 0);
    return &element->sum;
  } else return 0;
}

template <class CountT>
CountT *
NgramDecisionTree<CountT>::findCount(const VocabIndex *context,
				     VocabIndex word, Boolean &foundP)
{
  ElementType *element = this->find(context, foundP);
  if (foundP) {
    assert(element != 0);
    CountT *count = element->future.find(word);
    return count;
  } else return 0;
}

template <class CountT>
double
NgramDecisionTree<CountT>::scoreSplit(
    const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
    const SetType &setA, const SetType &setAbar, double *scoreNode)
{
  ElementType countsA, countsAbar;
  ElementType *data;
  VocabIndex word, wd;
  CountT *count, countNode;

  ELEM_ITER_T<VocabIndex, ElementType> iter(elements);
  while (data = iter.next(word)) {
    if (setA.find(word)) {
      FutureCountsIter cIter(data->future);
      while (count = cIter.next(wd)) {
	*(countsA.future.insert(wd)) += *count;
	countsA.sum += *count;
      }
    } else { 
      assert(setAbar.find(word));

      FutureCountsIter cIter(data->future);
      while (count = cIter.next(wd)) {
	*(countsAbar.future.insert(wd)) += *count;
	countsAbar.sum += *count;
      }
    }
  }

  // Compute log-likelihood
  double LogL = 0, nodeLogL = 0;
  VocabIter iterW(vocab);
  while (iterW.next(word)) {
    count = countsA.future.find(word);
    if (count) { 
      LogL += timesLog(*count); 
      countNode = *count; 
    } else countNode = 0;
    count = countsAbar.future.find(word);
    if (count) { 
      LogL += timesLog(*count); 
      countNode += *count; 
    }
    nodeLogL += timesLog(countNode);
  }
  LogL -= timesLog(countsA.sum) + timesLog(countsAbar.sum);
  nodeLogL -= timesLog(countsA.sum + countsAbar.sum);

  if (scoreNode) *scoreNode = nodeLogL;

  return LogL;
}
