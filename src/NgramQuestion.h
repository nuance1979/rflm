/*
 * NgramQuestion.h --
 *      Decision tree questions like:
 *        Does the word w_{-j} at position j in the history belong to a set S?
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _NgramQuestion_h_
#define _NgramQuestion_h_

#include "Vocab.h"
#include "Question.h"
#include "File.h"

#ifdef USE_SARRAY

#include "SArray.h"

typedef SArray<VocabIndex, char> SetType;
typedef SArrayIter<VocabIndex, char> SetTypeIter;

#else /* ! USE_SARRAY */

#include "LHash.h"

typedef LHash<VocabIndex, char> SetType;
typedef LHashIter<VocabIndex, char> SetTypeIter;

#endif /* USE_SARRAY */

class NgramQuestion: public Question<VocabIndex>
{
  friend class NgramQuestionIter;

 public:
  NgramQuestion();
  NgramQuestion(const SetType &set, unsigned int pos);
  virtual ~NgramQuestion();

  /*
   * Question interface
   */

  Boolean isTrue(const VocabIndex *context) const;// context is in reverse order
  Boolean readText(File &file, Vocab &vocab);
  void writeText(File &file, Vocab &vocab);

  inline unsigned int getPos() { return position; }
  inline SetType &getSet() { return setS; }

 protected:
  unsigned int position; // position \in [0..order-2]
  SetType setS;
};

#endif /* _NgramQuestion_h_ */
