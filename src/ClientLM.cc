/*
 * ClientLM.cc --
 *      Remote LM using Berkeley socket on the client side
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include "ClientLM.h"

ClientLM::ClientLM(Vocab &vocab, ClientSocket &sock)
  : socket(sock), LM(vocab)
{
}

ClientLM::~ClientLM()
{
}

LogP
ClientLM::wordProb1(VocabIndex word, const VocabIndex *context)
{
  char *query, *reply;
  LogP prob;
  unsigned trial = 0;
  
  query = prot.encodeParam(word, context);

  while (trial < 5) {

    socket.send(query);
    
    reply = socket.recv();  
    if (reply && prot.decodeProb(reply, prob)) 
      return prob;
    else 
      trial++;
  }

  cerr << "Failed to get prob from LM server.\n";
  exit(1);

  return 0; // Make g++ happy
}

LogP
ClientLM::wordProb(VocabIndex word, const VocabIndex *context)
{
  unsigned len;
  void *query, *reply;
  LogP prob;
  unsigned trial = 0;
  
  query = prot.encodeParam(word, context, len, 0);

  while (trial < 5) {
    
    socket.send(query, len);

    reply = socket.recv(len);
    if (reply && prot.decodeProb(reply, len, prob))
      return prob;
    else
      trial++;
  }

  cerr << "Failed to get prob from LM server.\n";
  exit(1);
  
  return 0; // Make g++ happy
}
