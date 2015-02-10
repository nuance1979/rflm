/*
 * rfngram-count --
 *	Create and manipulate random forest models
 *
 * Yi Su <suy@jhu.edu>
 *      based on SRI LM Toolkit's ngram-count
 *
 */

#ifndef lint
static char Copyright[] = "Copyright (c) 1995-2002 SRI International.  All Rights Reserved.";
static char RcsId[] = "@(#)$Id: rfngram-count.cc,v 1.5 2009/04/06 18:28:01 yisu Exp $";
#endif

#ifdef PRE_ISO_CXX
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#include <stdlib.h>
#include <locale.h>
#include <assert.h>

#include "option.h"
#include "version.h"
#include "File.h"
#include "Vocab.h"
#include "SubVocab.h"
#include "Ngram.h"
#include "VarNgram.h"
#include "TaggedNgram.h"
#include "SkipNgram.h"
#include "StopNgram.h"
#include "NgramStats.h"
#include "TaggedNgramStats.h"
#include "StopNgramStats.h"
#include "Discount.h"

#include "NgramDecisionTree.h"
#include "MultipassNgramDT.h"
#include "NgramDecisionTreeLM.h"
#include "ClientLM.h"

const unsigned maxorder = 9;		/* this is only relevant to the 
					 * the -gt<n> and -write<n> flags */
static int version = 0;
static char *filetag = 0;
static unsigned order = 3;
static unsigned debug = 0;
static char *textFile = 0;
static char *readFile = 0;
static int readWithMincounts = 0;

static unsigned writeOrder = 0;		/* default is all ngram orders */
static char *writeFile[maxorder+1];

static unsigned gtmin[maxorder+1] = {1, 1, 1, 2, 2, 2, 2, 2, 2, 2};
static unsigned gtmax[maxorder+1] = {5, 1, 7, 7, 7, 7, 7, 7, 7, 7};

static double cdiscount[maxorder+1] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static int ndiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int wbdiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int kndiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int ukndiscount[maxorder+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int knCountsModified = 0;
static int knCountsModifyAtEnd = 0;
static int interpolate[maxorder+1] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static char *gtFile[maxorder+1];
static char *knFile[maxorder+1];
static char *lmFile = 0;
static char *initLMFile = 0;

static char *vocabFile = 0;
static char *noneventFile = 0;
static char *writeVocab = 0;
static int memuse = 0;
static int recompute = 0;
static int sort1 = 0;
static int keepunk = 0;
static char *mapUnknown = 0;
static int tagged = 0;
static int toLower = 0;
static int trustTotals = 0;
static double prune = 0.0;
static unsigned minprune = 2;
static int useFloatCounts = 0;

static double varPrune = 0.0;

static int skipNgram = 0;
static double skipInit = 0.5;
static unsigned maxEMiters = 100;
static double minEMdelta = 0.001;

static char *stopWordFile = 0;
static char *metaTag = 0;

/*
 * Decision tree related
 */

static char *dtFile = 0;
static char *dtCounts = 0;  // for multipass dt
static char *dtHeldout = 0; // for multipass dt
static unsigned dtMaxNodes = 9000; // for multipass dt
static int noRandom = 0;
static int randInit = 0;
static int randSelect = 0;
static char *heldoutFile = 0;
static char *heldoutCount = 0;
static double dtPrune = 0.0;
static char *backoffFile = 0;
static char *backoffHost = 0;
static int backoffPort = 30000;
static unsigned internalOrder = 0;
static unsigned seed = 0;
static int shallowTree = 0;

static double alpha[maxorder+1] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5};

static Option options[] = {
    { OPT_TRUE, "version", &version, "print version information" },
    { OPT_UINT, "order", &order, "max ngram order" },
    { OPT_FLOAT, "varprune", &varPrune, "pruning threshold for variable order ngrams" },
    { OPT_UINT, "debug", &debug, "debugging level for LM" },
    { OPT_TRUE, "recompute", &recompute, "recompute lower-order counts by summation" },
    { OPT_TRUE, "sort", &sort1, "sort ngrams output" },
    { OPT_UINT, "write-order", &writeOrder, "output ngram counts order" },
    { OPT_STRING, "tag", &filetag, "file tag to use in messages" },
    { OPT_STRING, "text", &textFile, "text file to read" },
    { OPT_STRING, "read", &readFile, "counts file to read" },
    { OPT_TRUE, "read-with-mincounts", &readWithMincounts, "apply minimum counts when reading counts file" },

    { OPT_STRING, "write", &writeFile[0], "counts file to write" },
    { OPT_STRING, "write1", &writeFile[1], "1gram counts file to write" },
    { OPT_STRING, "write2", &writeFile[2], "2gram counts file to write" },
    { OPT_STRING, "write3", &writeFile[3], "3gram counts file to write" },
    { OPT_STRING, "write4", &writeFile[4], "4gram counts file to write" },
    { OPT_STRING, "write5", &writeFile[5], "5gram counts file to write" },
    { OPT_STRING, "write6", &writeFile[6], "6gram counts file to write" },
    { OPT_STRING, "write7", &writeFile[7], "7gram counts file to write" },
    { OPT_STRING, "write8", &writeFile[8], "8gram counts file to write" },
    { OPT_STRING, "write9", &writeFile[9], "9gram counts file to write" },

    { OPT_UINT, "gtmin", &gtmin[0], "lower GT discounting cutoff" },
    { OPT_UINT, "gtmax", &gtmax[0], "upper GT discounting cutoff" },
    { OPT_UINT, "gt1min", &gtmin[1], "lower 1gram discounting cutoff" },
    { OPT_UINT, "gt1max", &gtmax[1], "upper 1gram discounting cutoff" },
    { OPT_UINT, "gt2min", &gtmin[2], "lower 2gram discounting cutoff" },
    { OPT_UINT, "gt2max", &gtmax[2], "upper 2gram discounting cutoff" },
    { OPT_UINT, "gt3min", &gtmin[3], "lower 3gram discounting cutoff" },
    { OPT_UINT, "gt3max", &gtmax[3], "upper 3gram discounting cutoff" },
    { OPT_UINT, "gt4min", &gtmin[4], "lower 4gram discounting cutoff" },
    { OPT_UINT, "gt4max", &gtmax[4], "upper 4gram discounting cutoff" },
    { OPT_UINT, "gt5min", &gtmin[5], "lower 5gram discounting cutoff" },
    { OPT_UINT, "gt5max", &gtmax[5], "upper 5gram discounting cutoff" },
    { OPT_UINT, "gt6min", &gtmin[6], "lower 6gram discounting cutoff" },
    { OPT_UINT, "gt6max", &gtmax[6], "upper 6gram discounting cutoff" },
    { OPT_UINT, "gt7min", &gtmin[7], "lower 7gram discounting cutoff" },
    { OPT_UINT, "gt7max", &gtmax[7], "upper 7gram discounting cutoff" },
    { OPT_UINT, "gt8min", &gtmin[8], "lower 8gram discounting cutoff" },
    { OPT_UINT, "gt8max", &gtmax[8], "upper 8gram discounting cutoff" },
    { OPT_UINT, "gt9min", &gtmin[9], "lower 9gram discounting cutoff" },
    { OPT_UINT, "gt9max", &gtmax[9], "upper 9gram discounting cutoff" },

    { OPT_STRING, "gt", &gtFile[0], "Good-Turing discount parameter file" },
    { OPT_STRING, "gt1", &gtFile[1], "Good-Turing 1gram discounts" },
    { OPT_STRING, "gt2", &gtFile[2], "Good-Turing 2gram discounts" },
    { OPT_STRING, "gt3", &gtFile[3], "Good-Turing 3gram discounts" },
    { OPT_STRING, "gt4", &gtFile[4], "Good-Turing 4gram discounts" },
    { OPT_STRING, "gt5", &gtFile[5], "Good-Turing 5gram discounts" },
    { OPT_STRING, "gt6", &gtFile[6], "Good-Turing 6gram discounts" },
    { OPT_STRING, "gt7", &gtFile[7], "Good-Turing 7gram discounts" },
    { OPT_STRING, "gt8", &gtFile[8], "Good-Turing 8gram discounts" },
    { OPT_STRING, "gt9", &gtFile[9], "Good-Turing 9gram discounts" },

    { OPT_FLOAT, "cdiscount", &cdiscount[0], "discounting constant" },
    { OPT_FLOAT, "cdiscount1", &cdiscount[1], "1gram discounting constant" },
    { OPT_FLOAT, "cdiscount2", &cdiscount[2], "2gram discounting constant" },
    { OPT_FLOAT, "cdiscount3", &cdiscount[3], "3gram discounting constant" },
    { OPT_FLOAT, "cdiscount4", &cdiscount[4], "4gram discounting constant" },
    { OPT_FLOAT, "cdiscount5", &cdiscount[5], "5gram discounting constant" },
    { OPT_FLOAT, "cdiscount6", &cdiscount[6], "6gram discounting constant" },
    { OPT_FLOAT, "cdiscount7", &cdiscount[7], "7gram discounting constant" },
    { OPT_FLOAT, "cdiscount8", &cdiscount[8], "8gram discounting constant" },
    { OPT_FLOAT, "cdiscount9", &cdiscount[9], "9gram discounting constant" },

    { OPT_TRUE, "ndiscount", &ndiscount[0], "use natural discounting" },
    { OPT_TRUE, "ndiscount1", &ndiscount[1], "1gram natural discounting" },
    { OPT_TRUE, "ndiscount2", &ndiscount[2], "2gram natural discounting" },
    { OPT_TRUE, "ndiscount3", &ndiscount[3], "3gram natural discounting" },
    { OPT_TRUE, "ndiscount4", &ndiscount[4], "4gram natural discounting" },
    { OPT_TRUE, "ndiscount5", &ndiscount[5], "5gram natural discounting" },
    { OPT_TRUE, "ndiscount6", &ndiscount[6], "6gram natural discounting" },
    { OPT_TRUE, "ndiscount7", &ndiscount[7], "7gram natural discounting" },
    { OPT_TRUE, "ndiscount8", &ndiscount[8], "8gram natural discounting" },
    { OPT_TRUE, "ndiscount9", &ndiscount[9], "9gram natural discounting" },

    { OPT_TRUE, "wbdiscount", &wbdiscount[0], "use Witten-Bell discounting" },
    { OPT_TRUE, "wbdiscount1", &wbdiscount[1], "1gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount2", &wbdiscount[2], "2gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount3", &wbdiscount[3], "3gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount4", &wbdiscount[4], "4gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount5", &wbdiscount[5], "5gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount6", &wbdiscount[6], "6gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount7", &wbdiscount[7], "7gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount8", &wbdiscount[8], "8gram Witten-Bell discounting"},
    { OPT_TRUE, "wbdiscount9", &wbdiscount[9], "9gram Witten-Bell discounting"},

    { OPT_TRUE, "kndiscount", &kndiscount[0], "use modified Kneser-Ney discounting" },
    { OPT_TRUE, "kndiscount1", &kndiscount[1], "1gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount2", &kndiscount[2], "2gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount3", &kndiscount[3], "3gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount4", &kndiscount[4], "4gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount5", &kndiscount[5], "5gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount6", &kndiscount[6], "6gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount7", &kndiscount[7], "7gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount8", &kndiscount[8], "8gram modified Kneser-Ney discounting"},
    { OPT_TRUE, "kndiscount9", &kndiscount[9], "9gram modified Kneser-Ney discounting"},

    { OPT_TRUE, "ukndiscount", &ukndiscount[0], "use original Kneser-Ney discounting" },
    { OPT_TRUE, "ukndiscount1", &ukndiscount[1], "1gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount2", &ukndiscount[2], "2gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount3", &ukndiscount[3], "3gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount4", &ukndiscount[4], "4gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount5", &ukndiscount[5], "5gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount6", &ukndiscount[6], "6gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount7", &ukndiscount[7], "7gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount8", &ukndiscount[8], "8gram original Kneser-Ney discounting"},
    { OPT_TRUE, "ukndiscount9", &ukndiscount[9], "9gram original Kneser-Ney discounting"},

    { OPT_STRING, "kn", &knFile[0], "Kneser-Ney discount parameter file" },
    { OPT_STRING, "kn1", &knFile[1], "Kneser-Ney 1gram discounts" },
    { OPT_STRING, "kn2", &knFile[2], "Kneser-Ney 2gram discounts" },
    { OPT_STRING, "kn3", &knFile[3], "Kneser-Ney 3gram discounts" },
    { OPT_STRING, "kn4", &knFile[4], "Kneser-Ney 4gram discounts" },
    { OPT_STRING, "kn5", &knFile[5], "Kneser-Ney 5gram discounts" },
    { OPT_STRING, "kn6", &knFile[6], "Kneser-Ney 6gram discounts" },
    { OPT_STRING, "kn7", &knFile[7], "Kneser-Ney 7gram discounts" },
    { OPT_STRING, "kn8", &knFile[8], "Kneser-Ney 8gram discounts" },
    { OPT_STRING, "kn9", &knFile[9], "Kneser-Ney 9gram discounts" },

    { OPT_TRUE, "kn-counts-modified", &knCountsModified, "input counts already modified for KN smoothing"},
    { OPT_TRUE, "kn-modify-counts-at-end", &knCountsModifyAtEnd, "modify counts after discount estimation rather than before"},

    { OPT_TRUE, "interpolate", &interpolate[0], "use interpolated estimates"},
    { OPT_TRUE, "interpolate1", &interpolate[1], "use interpolated 1gram estimates"},
    { OPT_TRUE, "interpolate2", &interpolate[2], "use interpolated 2gram estimates"},
    { OPT_TRUE, "interpolate3", &interpolate[3], "use interpolated 3gram estimates"},
    { OPT_TRUE, "interpolate4", &interpolate[4], "use interpolated 4gram estimates"},
    { OPT_TRUE, "interpolate5", &interpolate[5], "use interpolated 5gram estimates"},
    { OPT_TRUE, "interpolate6", &interpolate[6], "use interpolated 6gram estimates"},
    { OPT_TRUE, "interpolate7", &interpolate[7], "use interpolated 7gram estimates"},
    { OPT_TRUE, "interpolate8", &interpolate[8], "use interpolated 8gram estimates"},
    { OPT_TRUE, "interpolate9", &interpolate[9], "use interpolated 9gram estimates"},

    { OPT_STRING, "lm", &lmFile, "LM to estimate" },
    { OPT_STRING, "init-lm", &initLMFile, "initial LM for EM estimation" },
    { OPT_TRUE, "unk", &keepunk, "keep <unk> in LM" },
    { OPT_STRING, "map-unk", &mapUnknown, "word to map unknown words to" },
    { OPT_STRING, "meta-tag", &metaTag, "meta tag used to input count-of-count information" },
    { OPT_TRUE, "float-counts", &useFloatCounts, "use fractional counts" },
    { OPT_TRUE, "tagged", &tagged, "build a tagged LM" },
    { OPT_TRUE, "skip", &skipNgram, "build a skip N-gram LM" },
    { OPT_FLOAT, "skip-init", &skipInit, "default initial skip probability" },
    { OPT_UINT, "em-iters", &maxEMiters, "max number of EM iterations" },
    { OPT_FLOAT, "em-delta", &minEMdelta, "min log likelihood delta for EM" },
    { OPT_STRING, "stop-words", &stopWordFile, "stop-word vocabulary for stop-Ngram LM" },

    { OPT_TRUE, "tolower", &toLower, "map vocabulary to lowercase" },
    { OPT_TRUE, "trust-totals", &trustTotals, "trust lower-order counts for estimation" },
    { OPT_FLOAT, "prune", &prune, "prune redundant probs" },
    { OPT_UINT, "minprune", &minprune, "prune only ngrams at least this long" },
    { OPT_STRING, "vocab", &vocabFile, "vocab file" },
    { OPT_STRING, "nonevents", &noneventFile, "non-event vocabulary" },
    { OPT_STRING, "write-vocab", &writeVocab, "write vocab to file" },
    { OPT_TRUE, "memuse", &memuse, "show memory usage" },
    { OPT_DOC, 0, 0, "the default action is to write counts to stdout" },

    { OPT_STRING, "dt", &dtFile, "decision tree file" },
    { OPT_STRING, "dt-counts", &dtCounts, "multipass decision tree counts" },
    { OPT_STRING, "dt-heldout", &dtHeldout, "multipass decision tree heldout counts" },
    { OPT_UINT, "dt-max-nodes", &dtMaxNodes, "multipass decision tree max nodes" },
    { OPT_TRUE, "no-random", &noRandom, "deterministiclly grow decision tree" },
    { OPT_TRUE, "random-init-only", &randInit, "randomize initialization only" },
    { OPT_TRUE, "random-select-only", &randSelect, "randomize selection only" },
    { OPT_STRING, "heldout-text", &heldoutFile, "heldout text file" },
    { OPT_STRING, "heldout-read", &heldoutCount, "heldout counts file" },
    { OPT_FLOAT, "dt-prune", &dtPrune, "decision tree pruning threshold" },
    { OPT_STRING, "backoff-lm", &backoffFile, "backoff LM" },
    { OPT_STRING, "backoff-host", &backoffHost, "backoff LM server host" },
    { OPT_INT, "backoff-port", &backoffPort, "backoff LM server port" },
    { OPT_UINT, "internal-order", &internalOrder, "internal order for feature ngram rflm" },
    { OPT_UINT, "seed", &seed, "seed for randomization" },
    { OPT_TRUE, "shallow-tree", &shallowTree, "grow a shallow tree with nodes less than dt-max-nodes" },

    { OPT_FLOAT, "alpha", &alpha[0], "randomizing constant" },
    { OPT_FLOAT, "alpha1", &alpha[1], "position 1 randomizing constant" },
    { OPT_FLOAT, "alpha2", &alpha[2], "position 2 randomizing constant" },
    { OPT_FLOAT, "alpha3", &alpha[3], "position 3 randomizing constant" },
    { OPT_FLOAT, "alpha4", &alpha[4], "position 4 randomizing constant" },
    { OPT_FLOAT, "alpha5", &alpha[5], "position 5 randomizing constant" },
    { OPT_FLOAT, "alpha6", &alpha[6], "position 6 randomizing constant" },
    { OPT_FLOAT, "alpha7", &alpha[7], "position 7 randomizing constant" },
    { OPT_FLOAT, "alpha8", &alpha[8], "position 8 randomizing constant" },
    { OPT_FLOAT, "alpha9", &alpha[9], "position 9 randomizing constant" }
};

int
main(int argc, char **argv)
{
    setlocale(LC_CTYPE, "");
    setlocale(LC_COLLATE, "");

    Boolean written = false;

    Opt_Parse(argc, argv, options, Opt_Number(options), 0);

    if (internalOrder == 0) internalOrder = order;

    if (version) {
	printVersion(RcsId);
	exit(0);
    }

    if (useFloatCounts + tagged + skipNgram +
	(stopWordFile != 0) + (varPrune != 0.0) > 1)
    {
	cerr << "fractional counts, variable, tagged, stop-word Ngram and skip N-gram models are mutually exclusive\n";
	exit(2);
    }

    if (randInit + randSelect + noRandom > 1) {
      cerr << "-random-init-only, -random-select-only, -no-random are mutually exclusive\n";
      exit(2);
    }
    RandType randType = noRandom ? NO_RAND : 
      randInit ? RAND_INIT : randSelect ? RAND_SELECT : BOTH;

    Vocab *vocab = tagged ? new TaggedVocab : new Vocab;
    assert(vocab);

    vocab->unkIsWord() = keepunk ? true : false;
    vocab->toLower() = toLower ? true : false;

    /*
     * Change unknown word string if requested
     */
    if (mapUnknown) {
	vocab->remove(vocab->unkIndex());
	vocab->unkIndex() = vocab->addWord(mapUnknown);
    }

    /*
     * Meta tag is used to input count-of-count information
     */
    if (metaTag) {
	vocab->metaTag() = metaTag;
    }

    SubVocab *stopWords = 0;

    if (stopWordFile != 0) {
	stopWords = new SubVocab(*vocab);
	assert(stopWords);
    }

    /*
     * The skip-ngram model requires count order one higher than
     * the normal model.
     */
    NgramStats *intStats =
	(stopWords != 0) ? new StopNgramStats(*vocab, *stopWords, order) :
	   tagged ? new TaggedNgramStats(*(TaggedVocab *)vocab, order) :
	      useFloatCounts ? 0 :
	         new NgramStats(*vocab, skipNgram ? order + 1 : order);
    NgramCounts<FloatCount> *floatStats =
	      !useFloatCounts ? 0 :
		 new NgramCounts<FloatCount>(*vocab, order);

#define USE_STATS(what) (useFloatCounts ? floatStats->what : intStats->what)

    if (useFloatCounts) {
	assert(floatStats != 0);
    } else {
	assert(intStats != 0);
    }

    USE_STATS(debugme(debug));

    if (vocabFile) {
	File file(vocabFile, "r");
	USE_STATS(vocab.read(file));
	USE_STATS(openVocab) = false;
    }

    if (stopWordFile) {
	File file(stopWordFile, "r");
	stopWords->read(file);
    }

    if (noneventFile) {
	/*
	 * create temporary sub-vocabulary for non-event words
	 */
	SubVocab nonEvents(USE_STATS(vocab));

	File file(noneventFile, "r");
	nonEvents.read(file);

	USE_STATS(vocab).addNonEvents(nonEvents);
    }

    if (readFile) {
	File file(readFile, "r");

	if (readWithMincounts) {
	    NgramCount minCounts[order];

	    /* construct min-counts array from -gtNmin options */
	    unsigned i;
	    for (i = 0; i < order && i < maxorder; i ++) {
		minCounts[i] = gtmin[i + 1];
	    }
	    for ( ; i < order; i ++) {
		minCounts[i] = gtmin[0];
	    }
	    USE_STATS(readMinCounts(file, order, minCounts));
	} else {
	    USE_STATS(read(file));
	}
    }

    if (textFile) {
	File file(textFile, "r");
	USE_STATS(countFile(file));
    }

    if (memuse) {
	MemStats memuse;
	USE_STATS(memStats(memuse));
	memuse.print();
    }

    if (recompute) {
	USE_STATS(sumCounts(order));
    }

    unsigned int i;
    for (i = 1; i <= maxorder; i++) {
	if (writeFile[i]) {
	    File file(writeFile[i], "w");
	    USE_STATS(write(file, i, sort1));
	    written = true;
	}
    }

    /*
     * While ngrams themselves can have order 0 (they will always be empty)
     * we need order >= 1 for LM estimation.
     */
    if (order == 0) {
	cerr << "LM order must be positive -- set to 1\n";
	order = 1;
    }

    /*
     * This stores the discounting parameters for the various orders
     * Note this is only needed when estimating an LM
     */
    Discount **discounts = new Discount *[order];
    assert(discounts != 0);

    for (i = 0; i < order; i ++) {
	discounts[i] = 0;
    }

    /*
     * Check for any Good Turing parameter files.
     * These have a dual interpretation.
     * If we're not estimating a new LM, simple WRITE the GT parameters
     * out.  Otherwise try to READ them from these files.
     */
    /*
     * Estimate discounting parameters 
     * Note this is only required if 
     * - the user wants them written to a file
     * - we also want to estimate a LM later
     */
    for (i = 1; i <= internalOrder; i++) {
	unsigned useorder = (i > maxorder) ? 0 : i;

	Discount *discount = 0;

	/*
	 * Choose discounting method to use
	 *
	 * Note: Test for ukndiscount[] before knFile[] so that combined use 
	 * of -ukndiscountN and -knfileN will do the right thing.
	 */
	if (ndiscount[useorder]) {
	    discount = new NaturalDiscount(gtmin[useorder]);
	    assert(discount);
	} else if (wbdiscount[useorder]) {
	    discount = new WittenBell(gtmin[useorder]);
	    assert(discount);
	} else if (cdiscount[useorder] != -1.0) {
	    discount = new ConstDiscount(cdiscount[useorder], gtmin[useorder]);
	    assert(discount);
	} else if (ukndiscount[useorder]) {
	    discount = new KneserNey(gtmin[useorder], knCountsModified, knCountsModifyAtEnd);
	    assert(discount);
	} else if (knFile[useorder] || kndiscount[useorder]) {
	    discount = new ModKneserNey(gtmin[useorder], knCountsModified, knCountsModifyAtEnd);
	    assert(discount);
	} else if (gtFile[useorder] || (i <= order && lmFile)) {
	    discount = new GoodTuring(gtmin[useorder], gtmax[useorder]);
	    assert(discount);
	}

	/*
	 * Now read in, or estimate the discounting parameters.
	 * Also write them out if no language model is being created.
	 */
	if (discount) {
	    discount->debugme(debug);

	    if (interpolate[0] || interpolate[useorder]) {
		discount->interpolate = true;
	    }

	    if (knFile[useorder] && (lmFile || dtFile)) {
		File file(knFile[useorder], "r");

		if (!discount->read(file)) {
		    cerr << "error in reading discount parameter file "
			 << knFile[useorder] << endl;
		    exit(1);
		}
	    } else if (gtFile[useorder] && (lmFile || dtFile)) {
		File file(gtFile[useorder], "r");

		if (!discount->read(file)) {
		    cerr << "error in reading discount parameter file "
			 << gtFile[useorder] << endl;
		    exit(1);
		}
	    } else {
		/*
		 * Estimate discount params, and write them only if 
		 * a file was specified, but no language model is
		 * being estimated.
		 */
		if (!(useFloatCounts ? discount->estimate(*floatStats, i) :
				       discount->estimate(*intStats, i)))
		{
		    cerr << "error in discount estimator for order "
			 << i << endl;
		    exit(1);
		}
		if (knFile[useorder]) {
		    File file(knFile[useorder], "w");
		    discount->write(file);
		    written = true;
		} else if (gtFile[useorder]) {
		    File file(gtFile[useorder], "w");
		    discount->write(file);
		    written = true;
		}
	    }

	    discounts[i-1] = discount;
	}
    }

    /*
     * Estimate a new model from the existing counts,
     * either using a default discounting scheme, or the GT parameters
     * read in from files
     */
    if (lmFile || dtFile) {
	Ngram *lm;
	NgramDecisionTree<NgramCount> *dt;
	LM *backoffLM;

	if (varPrune != 0.0) {
	    lm = new VarNgram(*vocab, order, varPrune);
	    assert(lm != 0);
	} else if (skipNgram) {
	    SkipNgram *skipLM =  new SkipNgram(*vocab, order);
	    assert(skipLM != 0);

	    skipLM->maxEMiters = maxEMiters;
	    skipLM->minEMdelta = minEMdelta;
	    skipLM->initialSkipProb = skipInit;

	    lm = skipLM;
	} else {
	    lm = (stopWords != 0) ? new StopNgram(*vocab, *stopWords, order) :
		       tagged ? new TaggedNgram(*(TaggedVocab *)vocab, order) :
			  new Ngram(*vocab, order);
	    assert(lm != 0);
	}
	
	/* decision tree language model */
	if (dtFile) {

	  dt = dtCounts? 
	    new MultipassNgramDT<NgramCount>(intStats, dtMaxNodes, randType):
	    new NgramDecisionTree<NgramCount>(*intStats, randType);
	  assert(dt);
	  
	  dt->debugme(debug);
	  
	  if (discounts[internalOrder-1] == 0) {
	    cerr << "need discount for order " << internalOrder;
	    cerr << " to build decision tree\n";
	    exit(1);
	  }
	  dt->setDiscount(*discounts[internalOrder-1]);
	  
	  ClientSocket *socket;
	  if (backoffHost) {
	    socket = new ClientSocket(backoffHost, backoffPort); 
	    assert(socket);
	    
	    backoffLM = new ClientLM(*vocab, *socket); assert(backoffLM);
	  } else if (backoffFile) {
	    File backoffIn(backoffFile, "r");
	    if (!lm->read(backoffIn)) {
	      cerr << "format error in backoff LM.\n";
	      exit(1);
	    }
	    backoffLM = lm;
	  } else {
	    if (!lm->estimate(*intStats, discounts)) {
	      cerr << "Backoff LM estimation failed.\n";
	      exit(1);
	    }
	    backoffLM = lm;
	  }
	  dt->setBackoffLM(*backoffLM);

	  dt->setAlphaArray(alpha);

	  if (shallowTree) {
	    MultipassNgramDT<NgramCount> *mdt 
	      = (MultipassNgramDT<NgramCount> *)dt;
	   
            mdt->setTreeFileName(dtFile); 
	    mdt->growShallow(seed);
	    cerr << "Finished growing shallow tree!\n";
	  } else {

	    if (heldoutFile == 0 && heldoutCount == 0) {
	      cerr << "need heldout file for pruning decision tree.\n";
	      exit(1);
	    }

	    NgramStats *heldoutStats = new NgramStats(*vocab, order);
	    assert(heldoutStats);
	    heldoutStats->openVocab = false;
	    if (heldoutFile) {
	      File heldoutIn(heldoutFile, "r");
	      heldoutStats->countFile(heldoutIn);
	    } else { // heldoutCount
	      File heldoutIn(heldoutCount, "r");
	      heldoutStats->read(heldoutIn);
	    }
	    dt->setHeldout(*heldoutStats);
	    
	    if (dtCounts == 0 && dtHeldout != 0) {
	      cerr << "Missing -dt-counts option\n";
	      exit(1);
	    }
	    if (dtCounts != 0 && dtHeldout == 0) {
	      cerr << "Missing -dt-heldout option\n";
	      exit(1);
	    }
	    
	    if (dtCounts != 0 && dtHeldout != 0) {
	      MultipassNgramDT<NgramCount> *mdt 
		= (MultipassNgramDT<NgramCount> *)dt;
	      mdt->setCountFileName(dtCounts);
	      mdt->setTreeFileName(dtFile);
	      mdt->setHeldoutFileName(dtHeldout);
	      if (internalOrder > 0) 
		mdt->setInternalOrder(internalOrder);
	    }
	    
	    dt->grow(seed);
	    cerr << "Finished growing tree!\n";
	    dt->printStats();

	    dt->prune(dtPrune);
	    dt->printStats();
	    cerr << "Finished pruning tree!\n";
	  }

	  File dtOut(dtFile, "w");
	  dt->write(dtOut);
	  
	  written = true;

#ifdef DEBUG	    
	  delete backoffLM;
	  delete socket;
	  delete heldoutStats;
#endif
	  if (lmFile) { // dtFile && lmFile
	    dt->loadCounts();
	    lm = new NgramDecisionTreeLM(*vocab, *dt, *discounts[order-1]);
	    assert(lm);
	  }
	}
	
	if (lmFile) {
	  /*
	   * Set debug level on LM object
	   */
	  lm->debugme(debug);
	  
	  /*
	   * Read initial LM parameters in case we're doing EM
	   */
	  if (initLMFile) {
	    File file(initLMFile, "r");
	    
	    if (!lm->read(file)) {
	      cerr << "format error in init-lm file\n";
	      exit(1);
	    }
	  }
	  
	  if (trustTotals) {
	    lm->trustTotals() = true;
	  }
	  if (!(useFloatCounts ? lm->estimate(*floatStats, discounts) :
		lm->estimate(*intStats, discounts)))
	    {
	      cerr << "LM estimation failed\n";
	      exit(1);
	    } else {
	    /*
	     * Remove redundant probs (perplexity increase below threshold)
	     */
	    if (prune != 0.0) {
	      lm->pruneProbs(prune, minprune);
	    }
	    
	    File file(lmFile, "w");
	    lm->write(file);
	  }
	  written = true;
	  
	// XXX: don't free the lm since this itself may take a long time
	// and we're going to exit anyways.
#ifdef DEBUG
	  delete lm;
	  delete dt;
#endif
	}
    }

    if (writeVocab) {
	File file(writeVocab, "w");
	vocab->write(file);
	written = true;
    }

    /*
     * If nothing has been written out so far, make it the default action
     * to dump the counts 
     *
     * Note: This will write the modified rather than the original counts
     * if KN discounting was used.
     */
    if (writeFile[0] || !written) {
	File file(writeFile[0] ? writeFile[0] : "-", "w");
	USE_STATS(write(file, writeOrder, sort1));
    }

#ifdef DEBUG
    /*
     * Free all objects
     */
    for (i = 0; i < order; i ++) {
	delete discounts[i];
	discounts[i] = 0;
    }
    delete [] discounts;

    delete intStats;
    delete floatStats;

    if (stopWords != 0) {
	delete stopWords;
    }

    delete vocab;
    return(0);
#endif /* DEBUG */

    exit(0);
}

