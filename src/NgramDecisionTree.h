/*
 * NgramDecisionTree.h --
 *      Decision tree for Ngram LM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _NgramDecisionTree_h_
#define _NgramDecisionTree_h_

#include "Vocab.h"
#include "DecisionTree.h"
#include "NgramStats.h"
#include "NgramQuestion.h"
#include "Discount.h"
#include "LM.h"

#ifdef USE_SARRAY

#define FutureCounts SArray<VocabIndex, CountT>
#define FutureCountsIter SArrayIter<VocabIndex, CountT>

#define ELEM_INDEX_T SArray
#define ELEM_ITER_T SArrayIter

#include "SArray.h"

#else /* ! USE_SARRAY */

#define FutureCounts LHash<VocabIndex, CountT>
#define FutureCountsIter LHashIter<VocabIndex, CountT>

#define ELEM_INDEX_T LHash
#define ELEM_ITER_T LHashIter

#include "LHash.h"

#endif /* USE_SARRAY */

// Helper class for node splitting
template <class CountT>
class Element {
 public:
  Element(): sum(0), min2Vocab(0), min3Vocab(0) {}
  ~Element() {}

  void clear() { future.clear(); sum = 0; }

  FutureCounts future;
  CountT sum;

  // For Modified Kneser-Ney smoothing
  Count min2Vocab;
  Count min3Vocab;

  void prepare() {
    VocabIndex wd;
    FutureCountsIter iter(future);
    
    Count observedVocab = 0;
    CountT totalCount = 0, *cnt;
    min2Vocab = 0; min3Vocab = 0;

    while ((cnt = iter.next(wd))) {
      if (*cnt > 0) observedVocab++;
      if (*cnt >= 2) min2Vocab++;
      if (*cnt >= 3) min3Vocab++;
      totalCount += *cnt;
    }
    assert(totalCount == sum);
    assert(observedVocab == future.numEntries());
  }
};

template <class CountT> class NgramDecisionTreeNode; // forward declaration

#define ElementType Element<CountT>

enum RandType { BOTH, RAND_SELECT, RAND_INIT, NO_RAND }; 

template <class CountT>
class NgramDecisionTree: public DecisionTree<VocabIndex, ElementType>,
  public LMStats
{
 public:
  NgramDecisionTree(NgramCounts<CountT> &cnts, RandType randType = BOTH,
		    double r = 0.5);
  virtual ~NgramDecisionTree();

  /*
   * LM Stats interface
   */
  
  unsigned int countSentence(const VocabString *words);
  unsigned int countSentence(const VocabIndex *words);
  unsigned int countSentence(const VocabString *words, const char *weight);
  void memStats(MemStats &stats);

  virtual Boolean read(File &file);
  virtual void write(File &file);

  virtual void grow(unsigned seed);  // Tree growing using exchage algorithm
  virtual void prune(const double threshold = 0.0); // CART-style pruning

  void prune(Discount &discount, LM &backoffLM, 
	     NgramCounts<CountT> &heldout, const double threshold = 0.0)
  { 
    setDiscount(discount); setBackoffLM(backoffLM); setHeldout(heldout);
    prune(threshold); 
  }

  void loadCounts() { loadCounts(cnts); }
  void loadCounts(NgramCounts<CountT> &counts); // Load counts into tree
  void prepareCounts(); // Prepare for Modified Kneser-Ney smoothing
  void clearCounts();
  void printCounts();

  void printStats();

  CountT *findCount(const VocabIndex *context, Boolean &foundP);
  CountT *findCount(const VocabIndex *context, VocabIndex word, 
		    Boolean &foundP);

  unsigned int getOrder() { return order; }
  void setDiscount(Discount &dist) { discount = &dist; assert(discount); }
  void setBackoffLM(LM &lm) { backoffLM = &lm; assert(backoffLM); }
  void setHeldout(NgramCounts<CountT> &cnts) 
  { heldout = &cnts; assert(heldout); }
  void setDumpFile(File &file) { dumpfile = &file; }
  void setInfoDumpFile(File &file) { infoDumpFile = &file; }
  File *getInfoDumpFile() { return infoDumpFile; }

  void setAlphaArray(double *r) { assert(r); alpha = r; }

  LM& getBackoffLM() { return *backoffLM; }

  Boolean getSeenHistory() { return seenHistory; }
  Boolean getSeenFuture() { return seenFuture; }

  LogP wordProb(Discount &discount, LM &backoffLM, VocabIndex word,
		const VocabIndex *context);
  LogP wordProb(VocabIndex word, const VocabIndex *context);
  LogP wordProb(ElementType *data, VocabIndex word, LogP backoffProb);
  LogP wordProb(VocabIndex word, const VocabIndex *context, LogP backoffProb);

 protected:
  unsigned int order;
  NgramCounts<CountT> &cnts;

  RandType randType; /* NO_RAND(3): no random at all; 
			RAND_INIT(2): random init only; 
			RAND_SELECT(1): random select only; 
			BOTH(0): both (default) */
  double _r; // Bernoulli(_r) for splitNodeRandomizer
  
  double *alpha; // Bernoulli(alpha[i]) for position-specific randomization

  File *dumpfile; // File for dumping histories in each leaf
  File *infoDumpFile; // File for dumping detailed info for ngram computation

  Boolean seenHistory;    // True if the history is in the tree
  Boolean seenFuture;  // True if the future word appears in training
  
  NgramCounts<CountT> *heldout;
  Discount *discount;
  LM *backoffLM;
  double threshold;

  Boolean splitNode(const NgramDecisionTreeNode<CountT> &node,
		   NgramDecisionTreeNode<CountT> *left,
		   NgramDecisionTreeNode<CountT> *right,
		   double &increase);
  void splitNodeRandomizer(unsigned int order, unsigned int *posSet,
			   unsigned int &posSetSize);
  
  void buildElements(const NgramDecisionTreeNode<CountT> &node,
		     const unsigned int pos,
		     ELEM_INDEX_T<VocabIndex, ElementType> &elements);

  void initSplit(const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
		 SetType &setA, ElementType &countsA,
		 SetType &setAbar, ElementType &countsAbar);
  void initSplitRandomizer(const ELEM_INDEX_T<VocabIndex, ElementType> 
			   &elements, SetType &setA, SetType &setAbar);

  unsigned moveElements(const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
			SetType &from, ElementType &countsFrom,
			SetType &to, ElementType &countsTo,
			double &accumulator);

  void writeTree(File &file, NgramDecisionTreeNode<CountT> *p);
  NgramDecisionTreeNode<CountT> *readTree(File &file, Boolean &ok);

  void computePotential(NgramDecisionTreeNode<CountT> *p,
			LogP &leavesLikelihood,
			CountT &leavesSum);
  void pruneTree(NgramDecisionTreeNode<CountT> *p);

  void clearCountsTree(NgramDecisionTreeNode<CountT> *p);
  void printCountsTree(NgramDecisionTreeNode<CountT> *p);

  void prepareCountsTree(NgramDecisionTreeNode<CountT> *p);

  /* Helper functions */
  double timesLog(CountT count); // function x*log(x) 
  unsigned cloneCounts(NgramCounts<CountT> &dest, NgramCounts<CountT> &source);
  void pourDownData(NgramDecisionTreeNode<CountT> *p);

  void printSet(const SetType &set);
  void printElement(const ElementType &element);

  /* Debugging */
  double scoreSplit(const ELEM_INDEX_T<VocabIndex, ElementType> &elements,
		    const SetType &setA, const SetType &setAbar, 
		    double *scoreNode = 0);
};

template <class CountT>
class NgramDecisionTreeNode: public DecisionTreeNode<VocabIndex, ElementType>
{
 public:
  NgramDecisionTreeNode(): counts(0), heldout(0), potential(0) {};
  virtual ~NgramDecisionTreeNode() { delete counts; delete heldout; };

  /* For growing */
  NgramCounts<CountT> *counts;
  /* For pruning */
  NgramCounts<CountT> *heldout;
  double potential;
};

#endif /* _NgramDecisionTree_h_ */
