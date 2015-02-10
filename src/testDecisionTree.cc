/* 
 * testDecisionTree --
 *      Test decision tree class
 */

#ifdef PRE_ISO_CXX
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#include <stdlib.h>
#include <stdio.h>

#include "Vocab.h"
#include "NgramStats.h"
#include "NgramDecisionTreeLM.h"
#include "NgramQuestion.h"
#include "Ngram.h"
#include "Discount.h"
#include "Prob.h"

int
main(int argc, char **argv)
{
  if (argc < 4) {
    cerr << "usage: testDecisionTree vocab text tree\n";
    exit(2);
  }

  cerr << "Reading vocab...";
  Vocab vocab;
  File vFile(argv[1], "r");
  unsigned howmany = vocab.read(vFile);  
  cerr << howmany << " words read.\n";
  vocab.unkIsWord() = true; // "-unk" option for ngram

  cerr << "Reading training text...";
  NgramStats stats(vocab, 3);
  stats.openVocab = false;
  File cFile(argv[2], "r");
  howmany = stats.countFile(cFile);
  cerr << howmany << " read.\n";
  cerr << "counts: " << stats.sumCounts() << endl;

  NgramDecisionTree<NgramCount> tree(stats);
  tree.debugme(0); tree.dout(cerr);
  cerr << "Reading tree...";
  File inFile(argv[3],"r");
  if (tree.read(inFile)) cerr << "done.\n";
  else cerr << "shit.\n";
  tree.printStats();
 
  cerr << "Loading counts...";
  tree.loadCounts();
  cerr << "done.\n";
  //  tree.printCounts();

  if (0) {
  NgramDecisionTree<NgramCount> tree(stats);
  tree.debugme(1); tree.dout(cerr);
  tree.grow(7);
  tree.printStats();
  }
  if (0) {
  cerr << "Loading counts...";
  tree.loadCounts();
  cerr << "done.\n";
  //  tree.printCounts();
  }

  if (0) {
  cerr << "Writing tree...";
  File outFile(argv[3], "w");
  tree.write(outFile);
  cerr << "done.\n";
  }
  cerr << "Estimating KN discount...";
  Discount **discounts = new Discount *[3];
  for (int i=1; i<=3; i++) {
    discounts[i-1] = new KneserNey((i==3)?2:1);
    if (discounts[i-1]->estimate(stats, i)) cerr << "order " << i << " done\n";
    else cerr << "shit\n";
  }

  cerr << "Estimating LM...";
  Ngram lm(vocab, 3);
  if (lm.estimate(stats, discounts)) cerr << "done.\n";
  else cerr << "shit\n";
  File backoffFile("/tmp/lm.kn", "w");
  lm.write(backoffFile);

  if (0) {
  tree.debugme(1);
  cerr << "Reading heldout text...";
  NgramStats heldout(vocab, 3);
  heldout.openVocab = false;
  File heldoutFile("/tmp/heldout", "r");
  howmany = heldout.countFile(heldoutFile);
  cerr << howmany << " read.\n";
  
  cerr << "Pruning...";
  tree.prune(*(discounts[2]), lm, heldout);
  cerr << "done.\n";
  File out("/tmp/tree.pruned", "w");
  tree.write(out);
  tree.printStats();

  cerr << "Loading counts...";
  tree.loadCounts();
  cerr << "done.\n";
  //  tree.printCounts();
  }
  cerr << "Computing ppl...";
  NgramDecisionTreeLM dtlm(vocab, tree, *(discounts[2]));
  dtlm.debugme(1); dtlm.dout(cerr);
  tree.debugme(3);
  tree.setBackoffLM(lm);

  cerr << "Without estimation...";
  //File test2("/tmp/train.1", "r"); 
  File test2("/tmp/test", "r");
  TextStats tstats2;
  dtlm.debugme(2); dtlm.dout(cerr);
  cerr << dtlm.pplFile(test2, tstats2); 
  cerr << " done.\n";
  cerr << tstats2;

  NgramStats testStats(vocab, 3);
  testStats.openVocab = false;
  File testfile("/tmp/test", "r");
  howmany = testStats.countFile(testfile);

  if (!dtlm.estimate(testStats, discounts)) cerr << "estimation failed.\n";
  File outLM("/tmp/lm", "w");
  dtlm.write(outLM);

  cerr << "With estimation...";
  //File test("/tmp/train.1", "r"); 
  File test("/tmp/test", "r");
  TextStats tstats;
  dtlm.debugme(2); dtlm.dout(cerr);
  cerr << dtlm.pplFile(test, tstats); 
  cerr << " done.\n";
  cerr << tstats;

  cerr << "Computing KN ppl...";
  TextStats tstats1;
  //File test1("/tmp/train.1", "r"); 
  File test1("/tmp/test", "r");
  cerr << lm.pplFile(test1, tstats1);
  cerr << " done.\n";
  cerr << tstats1;
  return 1;
}
