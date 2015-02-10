/*
 * DecisionTree.cc --
 *      Generic decision tree data structure
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include "DecisionTree.h"

#define INSTANTIATE_DECISIONTREE(KeyT, DataT)	\
  template class Question<KeyT>;		\
  template class DecisionTreeNode<KeyT, DataT>; \
  template class DecisionTree<KeyT, DataT>

template <class KeyT, class DataT>
DataT *
DecisionTree<KeyT, DataT>::find(const KeyT *keys, Boolean &foundP)
{
  DecisionTreeNode<KeyT, DataT> *p = root;
  assert(p != 0);
  
  foundP = true;
  while (!p->isLeaf()) {
    if (p->left && p->left->question && p->left->question->isTrue(keys)) 
      p = p->left;
    else if(p->right && p->right->question && p->right->question->isTrue(keys)) 
      p = p->right;
    else { foundP = false; break; }
  }

  if (foundP && p->data == 0) p->data = new DataT();
  return p->data;
}

template <class KeyT, class DataT>
DecisionTreeNode<KeyT, DataT> *
DecisionTree<KeyT, DataT>::find1(const KeyT *keys, Boolean &foundP)
{
  // Exactly the same procedure as in find() but return the node
  DecisionTreeNode<KeyT, DataT> *p = root;
  assert(p != 0);
  
  foundP = true;
  while (!p->isLeaf()) {
    if (p->left && p->left->question && p->left->question->isTrue(keys)) 
      p = p->left;
    else if(p->right && p->right->question && p->right->question->isTrue(keys)) 
      p = p->right;
    else { foundP = false; break; }
  }

  return p;
}


template <class KeyT, class DataT>
void
DecisionTree<KeyT, DataT>::freeTree(DecisionTreeNode<KeyT, DataT> *node)
{
  if (node) {
    freeTree(node->left);
    freeTree(node->right);
    delete node;
  }
}

template <class KeyT, class DataT>
void
DecisionTree<KeyT, DataT>::treeStats(TreeStats &tstats,
				     DecisionTreeNode<KeyT, DataT> *node)
{
  if (!node) {
    tstats.numNodes = 0; tstats.numLeaves = 0; tstats.depth = 0;
  } else if (node->isLeaf()) {
    tstats.numNodes = 1; tstats.numLeaves = 1; tstats.depth = 1;
  } else {
    treeStats(tstats, node->left);
    TreeStats t;
    treeStats(t, node->right);

    tstats.numNodes += t.numNodes + 1;
    tstats.numLeaves += t.numLeaves;
    tstats.depth = (tstats.depth > t.depth) ? tstats.depth + 1 : t.depth + 1;
  }
}
