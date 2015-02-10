/*
 * LMServer.cc --
 *      LM server using Berkeley socket to answer class ClientLM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include "LMServer.h"

#define DEBUG_LM_SERVER 1

LMServer::LMServer(LM &model, ServerSocket &sock)
  : lm(model), socket(sock)
{
}

LMServer::~LMServer()
{
}

void
LMServer::run1()
{
  char *query, *reply;
  LogP prob;
  VocabIndex word, *context = new VocabIndex[maxWordsPerLine];
  Boolean ok;

  cerr << "running...";
  while (1) {
    ServerSocket s;
    socket.accept(s);

    while (1) {
      if ((query = s.recv()) == 0) break;

      if (debug(DEBUG_LM_SERVER))
	dout() << "received: " << query << endl;

      ok = prot.decodeParam(query, word, context);

      if (!ok) {
	if (debug(DEBUG_LM_SERVER)) dout() << "decoding failed.\n"; 
	break; 
      }
      prob = 0; cerr << word << " ";
      for (int i=0; i<10; i++) cerr << context[i] << " ";
      cerr << endl;
      prob = lm.wordProb(word, context);
      reply = prot.encodeProb(prob);

      if (reply == 0) { 
	if (debug(DEBUG_LM_SERVER)) dout() << "encoding failed.\n"; 
	break; 
      }

      if (debug(DEBUG_LM_SERVER)) dout() << "sending: " << reply << endl;
      if (!s.send(reply)) break;
    }
  }
}

void
LMServer::run()
{
  void *query, *reply;
  unsigned len;
  LogP prob;
  VocabIndex word, *context = new VocabIndex[maxWordsPerLine];
  Boolean ok;
  unsigned i;

  cerr << "running...";
  while (1) {
    ServerSocket s;
    socket.accept(s);

    while (1) {
      if ((query = s.recv(len)) == 0) break;
      
      if (debug(DEBUG_LM_SERVER)) {
	dout() << "received: ";
	for (i=0; i<len; i++) 
	  dout() << (((char *)query)[i] - '\0') << " ";
	dout() << endl;
      }

      ok = prot.decodeParam(query, len, word, context);
      
      if (!ok) {
	if (debug(DEBUG_LM_SERVER)) dout() << "decoding failed.\n";
	break;
      }

      prob = lm.wordProb(word, context);
      reply = prot.encodeProb(prob, len);

      if (reply == 0) {
	if (debug(DEBUG_LM_SERVER)) dout() << "encoding failed.\n";
	break;
      }

      if (debug(DEBUG_LM_SERVER)) {
	dout() << "sending: ";
	for (i=0; i<len; i++) 
	  dout() << (((char *)reply)[i] - '\0') << " ";
	dout() << endl;
      }
      if (!s.send(reply, len)) break;
    }
  }
}
