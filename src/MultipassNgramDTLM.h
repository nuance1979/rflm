/*
 * MultipassNgramDTLM.h --
 *      Multi-pass decision tree language models based on ngram stats
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _MultipassNgramDTLM_h_
#define _MultipassNgramDTLM_h_

#include "MultipassNgramDT.h"
#include "NgramDecisionTreeLM.h"

class MultipassNgramDTLM: public NgramDecisionTreeLM
{
 public:
  MultipassNgramDTLM(Vocab &vocab, MultipassNgramDT<NgramCount> &dt,
		     Discount &dist);
  virtual ~MultipassNgramDTLM();

  /*
   * LM interface
   */
  virtual LogP wordProb(VocabIndex word, const VocabIndex *context);

  /*
   * Estimation
   */
  virtual Boolean estimate(NgramStats &stats, Discount **discounts);

 protected:
  template <class CountType>
    Boolean estimate2(NgramCounts<CountType> &stats, Discount **discounts);
};

#endif /* _MultipassNgramDTLM_h_ */
