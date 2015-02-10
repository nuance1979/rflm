/*
 * Question.h --
 *      Generic decision tree question
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#ifndef _Question_h_
#define _Question_h_

#include "File.h"
#include "Boolean.h"

template <class KeyT>
class Question
{
 public:
  Question() {};
  virtual ~Question() {};

  virtual Boolean isTrue(const KeyT *keys) const = 0;
};

#endif /* _Question_h_ */
