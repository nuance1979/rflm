/*
 * LMServer.h --
 *      LM server using Berkeley socket to answer class ClientLM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _LMServer_h_
#define _LMServer_h_

#include "LM.h"
#include "ServerSocket.h"
#include "Protocol.h"
#include "Debug.h"

class LMServer : public Debug
{
 public:
  LMServer(LM &model, ServerSocket &sock);
  virtual ~LMServer();
  
  void run();
  void run1();

 private:
  LM &lm;
  ServerSocket &socket;

  Protocol prot;
};

#endif /* _LMServer_h_ */
