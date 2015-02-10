/*
 * Protocol.h --
 *     Encode/decode messages passing between LM server and client LM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _Protocol_h_
#define _Protocol_h_

#include "Vocab.h"
#include "Boolean.h"
#include "Prob.h"
#include "File.h"

class Protocol
{
 public:
  Protocol();
  ~Protocol();

  // Codec for C string (zero ended) message
  char *encodeParam(VocabIndex word, const VocabIndex *context, 
		    unsigned contextLength = 0);
  Boolean decodeParam(char *msg, VocabIndex &word, 
		      VocabIndex *context,
		      unsigned maxContextLength = maxWordsPerLine);
  
  char *encodeProb(LogP prob);
  Boolean decodeProb(char *msg, LogP &prob);

  // Codec for anything (with len)
  void *encodeParam(VocabIndex word, const VocabIndex *context, unsigned &len,
		    unsigned contextLength = 0);
  Boolean decodeParam(void *msg, unsigned len, VocabIndex &word,
		      VocabIndex *context,
		      unsigned maxContextLength = maxWordsPerLine);

  void *encodeProb(LogP prob, unsigned &len);
  Boolean decodeProb(void *msg, unsigned len, LogP &prob);
  
 private:
  void *buf;
  unsigned bufLen;
};

#endif /* _Protocol_h_ */
