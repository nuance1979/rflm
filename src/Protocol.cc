/*
 * Protocol.cc --
 *     Encode/decode messages passing between LM server and client LM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include "Protocol.h"

const unsigned MAXBUFSIZE = 5000;

Protocol::Protocol()
  : buf(malloc(MAXBUFSIZE)), bufLen(MAXBUFSIZE)
{
}

Protocol::~Protocol()
{
  free(buf);
}

char *
Protocol::encodeParam(VocabIndex word, const VocabIndex *context,
		      unsigned contextLength)
{
  int ret;
  unsigned bufOffset = 0;
  
  ret = snprintf((char *)buf, bufLen, "%u", word);
  while (ret + 1 > bufLen) {
    bufLen *= 2;
    buf = realloc(buf, bufLen); assert(buf);
    ret = snprintf((char *)buf, bufLen, "%u", word);
  }
  bufOffset = ret;

  unsigned i, clen = Vocab::length(context); assert(contextLength <= clen);
  clen = (contextLength > 0)? contextLength : clen;

  for (i=0; i<clen; i++) {
    ret = snprintf((char *)buf + bufOffset, bufLen - bufOffset, 
		   " %u", context[i]);
    while (ret + 1 > bufLen - bufOffset) {
      bufLen *= 2;
      buf = realloc(buf, bufLen); assert(buf);
      ret = snprintf((char *)buf + bufOffset, bufLen - bufOffset, 
		     " %u", context[i]);
    }
    bufOffset += ret;
  }

  // snprintf() would guarantee tailer '\0'
  return (char *)buf;
}

Boolean
Protocol::decodeParam(char *msg, VocabIndex &word, VocabIndex *context,
		      unsigned maxContextLength)
{
  if (msg == 0) return false;

  char *tok = strtok(msg, wordSeparators); cerr << "word: " << tok << endl;
  if (tok == 0 || sscanf(tok, "%u", &word) != 1) return false;

  int i;
  for (i = 0, tok = strtok(0, wordSeparators); 
       i < maxContextLength - 1 && tok != 0; 
       i++, tok = strtok(0, wordSeparators)) {
    cerr << "tok: " << tok << endl;
    if (sscanf(tok, "%u", &(context[i])) != 1) return false;
  }

  // make LM::wordProb() happy
  if (context[i - 1] != Vocab_None) context[i] = Vocab_None;
  
  return true;
}

char *
Protocol::encodeProb(LogP prob)
{
  snprintf((char *)buf, bufLen, "%f", prob);

  return (char *)buf;
}

Boolean
Protocol::decodeProb(char *msg, LogP &prob)
{
  if (msg == 0) return false;

  return (sscanf(msg, "%f", &prob) == 1);
}

void *
Protocol::encodeParam(VocabIndex word, const VocabIndex *context, unsigned &len,
		      unsigned contextLength)
{
  unsigned clen = Vocab::length(context); assert(contextLength <= clen);
  clen = (contextLength > 0)? contextLength : clen;
  len = sizeof(VocabIndex) * (1 + clen);
  
  while (len > bufLen) {
    bufLen *= 2;
    buf = realloc(buf, bufLen); assert(buf);
  }
  
  unsigned bufOffset = 0;

  memcpy(buf, &word, sizeof(VocabIndex));
  bufOffset += sizeof(VocabIndex);

  memcpy((char *)buf + bufOffset, context, sizeof(VocabIndex) * clen);

  return buf;
}

Boolean
Protocol::decodeParam(void *msg, unsigned len, VocabIndex &word,
		      VocabIndex *context, unsigned maxContextLength)
{
  if (msg == 0) return false;

  if (len % sizeof(VocabIndex) != 0 || len < sizeof(VocabIndex)) return false;
  
  unsigned clen = len / sizeof(VocabIndex) - 1;
  if (clen + 1 > maxContextLength) return false;

  unsigned bufOffset = 0;

  memcpy(&word, msg, sizeof(VocabIndex));
  bufOffset += sizeof(VocabIndex);

  memcpy(context, (char *)msg + bufOffset, sizeof(VocabIndex) * clen);

  // Make LM::wordProb() happy
  if (context[clen - 1] != Vocab_None) context[clen] = Vocab_None;

  return true;
}

void *
Protocol::encodeProb(LogP prob, unsigned &len)
{
  memcpy(buf, &prob, sizeof(LogP));
 
  len = sizeof(LogP);
  return buf;
}

Boolean
Protocol::decodeProb(void *msg, unsigned len, LogP &prob)
{
  if (msg == 0 || len != sizeof(LogP)) return false;

  memcpy(&prob, msg, len);

  return true;
}
