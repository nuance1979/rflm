/*
 * NgramQuestion.cc --
 *      Decision tree questions like:
 *        Does the word w_{-j} at position j in the history belong to a set S?
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include <assert.h>
#include "NgramQuestion.h"
#include "NgramStats.h"

#ifdef USE_SARRAY

#include "SArray.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_SARRAY(VocabIndex, char);
#endif

#else /* ! USE_SARRAY */

#include "LHash.cc"

#ifdef INSTANTIATE_TEMPLATES
INSTANTIATE_LHASH(VocabIndex, char);
#endif

#endif /* USE_SARRAY */

NgramQuestion::NgramQuestion()
  : position(0)
{
}

NgramQuestion::NgramQuestion(const SetType &set, unsigned int pos)
  : position(pos)
{
  // Copy constructor
  VocabIndex wd;
  SetTypeIter iter(set);
  while (iter.next(wd)) setS.insert(wd);
}

NgramQuestion::~NgramQuestion()
{
}

inline Boolean
NgramQuestion::isTrue(const VocabIndex *context) const
{
  assert(context != 0);

  return (setS.find(context[position]) != 0);
}

Boolean
NgramQuestion::readText(File &file, Vocab &vocab)
{
  char *line;
  line = file.getline();

  unsigned int size = vocab.numWords();
  VocabString words[size + 1];
  unsigned int howmany = Vocab::parseWords(line, words, size + 1); 
  if (howmany == size + 1) return false;

  if (sscanf(words[0], "%d", &position) != 1) return false;
  setS.clear();
  VocabIndex index;
  for (unsigned i=1; i<howmany; i++) {
    index = vocab.getIndex(words[i]);
    if (setS.insert(index) == 0) return false;
  }
  return true;
}

void
NgramQuestion::writeText(File &file, Vocab &vocab)
{
  file.fprintf("%d", position);
  VocabIndex wd;
  SetTypeIter iter(setS);
  while (iter.next(wd))
    file.fprintf(" %s", vocab.getWord(wd)); 
  file.fprintf("\n");
}

