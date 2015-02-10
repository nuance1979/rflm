/*
 * MultipassNgramDT.h --
 *      Multi-pass decision tree for Ngram LM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _MultipassNgramDT_h_
#define _MultipassNgramDT_h_

#include "Ngram.h"
#include "NgramDecisionTree.h"

#define PruneStatsType PruneStats<CountT>

#ifdef USE_SARRAY

#define StatsList SArray<int, PruneStatsType>
#define IdSetType SArray<int, char>
#define IdSetTypeIter SArrayIter<int, char>

#include "SArray.h"

#else /* ! USE_SARRAY */

#define StatsList LHash<int, PruneStatsType>
#define IdSetType LHash<int, char>
#define IdSetTypeIter LHashIter<int, char>

#include "LHash.h"

#endif /* USE_SARRAY */

// Helper class for multi-pass pruning
template <class CountT>
class PruneStats {
 public:
  PruneStats(): leavesLikelihood(0), leavesSum(0) {}
  ~PruneStats() {}

  LogP leavesLikelihood;
  CountT leavesSum;
};

template <class CountT> class MultipassNgramDTNode; // forward declaration

template <class CountT>
class MultipassNgramDT : public NgramDecisionTree<CountT>
{
 public:
  /* NOTE: pcnts will be deleted after calling grow()!!! */
  MultipassNgramDT(NgramCounts<CountT> *pcnts, unsigned maxNodes = 9000, 
		   RandType randType = BOTH, double r = 0.5);
  virtual ~MultipassNgramDT();

  void setTreeFileName(const char *fn);
  void setCountFileName(const char *fn);
  void setHeldoutFileName(const char *fn);
  void setTestFileName(const char *fn);

  /* Internal order to support feature ngram RFLM 
     e.g., if counts file looks like this:
            feat2 feat1 w2 w1 w0 cnt
     then its order = 5, internalOrder = 3.
   */
  void setInternalOrder(const unsigned order) 
  { internalOrder = (order < this->order)? order : this->order; }
  unsigned getInternalOrder() { return internalOrder; }

  /* class NgramDecisionTree interface */

  /* NOTE: grow() will delete this->pcnts and this->heldout 
     to save memory!!! */
  virtual void grow(unsigned seed); 
                       /* Call setTreeFileName(), setCountFileName(), 
			  setHeldoutFileName(),
			  setHeldout(), setBackoffLM() and setDiscount()
			  before calling grow()!!! */

  void growShallow(unsigned seed); /* Grow a shallow tree with number of nodes
				      below maxNodes */

  virtual void prune(const double threshold); /* Call setDiscount()
						 setBackoffLM(), setHeldout()
						 before calling prune()!!! */

  virtual Boolean read(File &file);
  virtual void write(File &file);

  /* Compute probs of a bunch of ngrams in a batch mode;
     Call setTestFileName(), setCountFileName() and setTreeFileName() 
     before calling!!! */

  void eval(NgramCounts<CountT> &testStats, Ngram &lm);
  
 protected:
  NgramCounts<CountT> *pcnts;
  
  unsigned numSubtrees;
  unsigned numLeaves;
  unsigned numSteps;

  unsigned maxNodes;

  char *treeFileName;
  char *countFileName;
  char *heldoutFileName;
  char *testFileName;

  unsigned internalOrder;

  TreeStats trStats;
  
  StatsList *slist; // Stats for multipass pruning

  int getSubtreeId();
  int getLeafId();

  /* Preorder traverse tree to do a couple of things given different params.
     Used in both training and testing to keep the same order of 
     subtrees/leaves. */

  Boolean dump(File *treeFile, File *countFile, File *hCountFile, int rootId,
	       IdSetType *idList = 0);
  Boolean dumpTree(File *treefile, File *countFile, File *hCountFile, 
		   MultipassNgramDTNode<CountT> *p, IdSetType *idList);

  /* Load the tree and return the rootId. If loadByOrder is true, try 
     to load the tree whose root id is rootId. If failed, put the 
     actual root id encountered in rootId and return true. Return false
     when format error. rootId = 0 when no rootId has been read.

     NOTE: save the effort of loading an unwanted tree. */

  Boolean load(File &treeFile, File *pCountFile, int &rootId,
	       Boolean loadByOrder = false);
  MultipassNgramDTNode<CountT> *loadTree(File &treeFile, File *pCountFile, 
					 Boolean &ok);

  Boolean growStep(File &treeFile, File &countFile, File &outCountFile,
		   File &hCountFile, File &outHCountFile, unsigned maxNodes);

  /* Core tree growing algorithm */
  Boolean growSubtree(int rid, File &treeFile, File &outCountFile, 
		      File &outHCountFile, unsigned maxNodes);

  Boolean growSubtreeShallow(int rid, File &treeFile, unsigned maxNodes);

  void compPtnlAndPrune(MultipassNgramDTNode<CountT> *p, double threshold,
			LogP &leavesLikelihood, CountT &leavesSum);

  void postPruneProcess(int &rootId);
  void postPruneTree(MultipassNgramDTNode<CountT> *p);

  Boolean evalStep(File &treeFile, File &countFile, File &outCountFile, 
		   File &tCountFile, File &outTCountFile, Ngram &lm);

  Boolean evalSubtree(int rid, File &outCountFile, File &outTCountFile, 
		      Ngram &lm);

  void loadAndPourCounts();
  unsigned computeProbTree(MultipassNgramDTNode<CountT> *p, Ngram &lm);

  /* Helper functions */
  char *buf;
  unsigned bufLen;

  char *getFN(const char *base, int ind, const char *suffix = 0);
};

template <class CountT>
class MultipassNgramDTNode: public NgramDecisionTreeNode<CountT>
{
 public:
  MultipassNgramDTNode(): id(0), LogL(0), heldoutSum(0) {};
  virtual ~MultipassNgramDTNode() {};

  int id; /* id < 0 for subtree;
	     id == 0 for internal node;
             id > 0  for leaf; 
	     subtree -1 is global root. */

  LogP LogL;
  CountT heldoutSum;
};

#endif /* _MultipassNgramDT_h_ */
