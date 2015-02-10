/*
 * DecisionTree.h --
 *      Generic decision tree data structure
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _DecisionTree_h_
#define _DecisionTree_h_

#include "Question.h"
#include "Debug.h"

class TreeStats
{
 public:
  TreeStats(): numNodes(0), numLeaves(0), depth(0) {}
  ~TreeStats() {}

  unsigned numNodes;
  unsigned numLeaves;
  unsigned depth;
};

template <class KeyT, class DataT> 
  class DecisionTreeNode; // forward declaration

template <class KeyT, class DataT>
class DecisionTreeNode
{
 public:
  DecisionTreeNode(): left(0), right(0), question(0), data(0) {}
  virtual ~DecisionTreeNode() { delete question; delete data; }
  
  inline Boolean isLeaf() { return (left == 0 && right == 0); }

  Question<KeyT> *question;
  DataT *data;

  DecisionTreeNode<KeyT, DataT> *left;
  DecisionTreeNode<KeyT, DataT> *right;
};

template <class KeyT, class DataT>
class DecisionTree
{
 public:
  DecisionTree(): root(0) {};
  virtual ~DecisionTree() { freeTree(root); };

  DataT *find(const KeyT *keys, Boolean &foundP);

  DecisionTreeNode<KeyT, DataT> *find1(const KeyT *keys, Boolean &foundP);
  
  void freeTree(DecisionTreeNode<KeyT, DataT> *node);

  void treeStats(TreeStats &tstats, DecisionTreeNode<KeyT, DataT> *node);

 protected:
  DecisionTreeNode<KeyT, DataT> *root;
};

#endif /* _DecisionTree_h_ */
