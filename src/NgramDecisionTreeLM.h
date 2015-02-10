/*
 * NgramDecisionTreeLM.h --
 *      Decision tree language models based on ngram stats
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _NgramDecisionTreeLM_h_
#define _NgramDecisionTreeLM_h_

#include "Ngram.h"
#include "NgramDecisionTree.h"
#include "Discount.h"

class NgramDecisionTreeLM: public Ngram
{
 public:
  NgramDecisionTreeLM(Vocab &vocab, NgramDecisionTree<NgramCount> &dt,
		      Discount &dist);
  virtual ~NgramDecisionTreeLM();
  
  /*
   * LM interface
   */
  virtual LogP wordProb(VocabIndex word, const VocabIndex *context);

  /*
   * Estimation
   */
  virtual Boolean estimate(NgramStats &stats, Discount **discounts);
  virtual Boolean estimate(NgramCounts<FloatCount> &stats,
			   Discount **discounts);

 protected:
  NgramDecisionTree<NgramCount> *tree;
  Boolean estimated;

  template <class CountType>
    Boolean estimate2(NgramCounts<CountType> &stats, Discount **discounts);
};

#endif /* _NgramDecisionTreeLM_h_ */
