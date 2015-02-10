/*
 * ClientLM.h --
 *      Remote LM using Berkeley socket on the client side
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _ClientLM_h_
#define _ClientLM_h_

#include "LM.h"
#include "ClientSocket.h"
#include "Protocol.h"

class ClientLM : public LM
{
 public:
  ClientLM(Vocab &vocab, ClientSocket &sock);
  virtual ~ClientLM();

  /*
   * LM interface
   */

  virtual LogP wordProb(VocabIndex word, const VocabIndex *context);

  virtual LogP wordProb1(VocabIndex word, const VocabIndex *context);

 private:
  ClientSocket &socket;
  
  Protocol prot;
};

#endif /* _ClientLM_h_ */
