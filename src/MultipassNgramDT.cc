/*
 * MultipassNgramDT.cc --
 *      Multi-pass decision tree for Ngram LM
 *
 * Yi Su <suy@jhu.edu>
 *
 */

#include <stdio.h>
#include "MultipassNgramDT.h"

#include "NgramDecisionTree.cc"

#define DEBUG_GROW 1

#define MNODE(p) ((MultipassNgramDTNode<CountT> *)(p))
#define MROOT MNODE(this->root)

#define INSTANTIATE_MULTIPASSNGRAMDT_DSTRUCT(CountT)	\
  template class PruneStats<CountT>; \
  template class MultipassNgramDT<CountT> \

#ifdef USE_SARRAY

#include "SArray.cc"

#define INSTANTIATE_MULTIPASSNGRAMDT(CountT)	\
  INSTANTIATE_SARRAY(int, PruneStats<CountT>);  \
  INSTANTIATE_SARRAY(int, char); \
  INSTANTIATE_MULTIPASSNGRAMDT_DSTRUCT(CountT)

#else /* ! USE_SARRAY */

#include "LHash.cc"

#define INSTANTIATE_MULTIPASSNGRAMDT(CountT)	\
  INSTANTIATE_LHASH(int, PruneStats<CountT>);  \
  INSTANTIATE_LHASH(int, char); \
  INSTANTIATE_MULTIPASSNGRAMDT_DSTRUCT(CountT)

#endif /* USE_SARRAY */

template <class CountT>
MultipassNgramDT<CountT>::MultipassNgramDT(NgramCounts<CountT> *pcnts,
					   unsigned maxNodes,
					   RandType randType, double r)
  : pcnts(pcnts), NgramDecisionTree<CountT>(*pcnts, randType, r),
    numSubtrees(0), numLeaves(0),
    treeFileName(0), countFileName(0), heldoutFileName(0), testFileName(0), 
    bufLen(100), buf((char *)malloc(100)), maxNodes(maxNodes),
    internalOrder(pcnts->getorder())
{
  assert(pcnts); // Do not call this with pcnts == 0!!!
}

template <class CountT>
MultipassNgramDT<CountT>::~MultipassNgramDT()
{
  if (treeFileName) free(treeFileName);
  if (countFileName) free(countFileName);
  if (heldoutFileName) free(heldoutFileName);
  if (testFileName) free(testFileName);

  free(buf);
}

template <class CountT>
int
MultipassNgramDT<CountT>::getSubtreeId()
{
  numSubtrees++;
  return -numSubtrees;
}

template <class CountT>
int
MultipassNgramDT<CountT>::getLeafId()
{
  numLeaves++;
  return numLeaves;
}

template <class CountT>
void
MultipassNgramDT<CountT>::setTreeFileName(const char *fn)
{
  treeFileName = (char *)realloc(treeFileName, strlen(fn)+1);
  strcpy(treeFileName, fn);
}

template <class CountT>
void
MultipassNgramDT<CountT>::setCountFileName(const char *fn)
{
  countFileName = (char *)realloc(countFileName, strlen(fn)+1);
  strcpy(countFileName, fn);
}

template <class CountT>
void
MultipassNgramDT<CountT>::setHeldoutFileName(const char *fn)
{
  heldoutFileName = (char *)realloc(heldoutFileName, strlen(fn)+1);
  strcpy(heldoutFileName, fn);
}

template <class CountT>
void
MultipassNgramDT<CountT>::setTestFileName(const char *fn)
{
  testFileName = (char *)realloc(testFileName, strlen(fn)+1);
  strcpy(testFileName, fn);
}

template <class CountT>
char *
MultipassNgramDT<CountT>::getFN(const char *base, int ind, const char *suffix)
{
  unsigned len = strlen(base) + 20;
  if (suffix) len += strlen(suffix);
  while (bufLen < len) {
    bufLen *= 2;
    buf = (char *)realloc(buf, bufLen); assert(buf);
  }
  
  if (suffix) 
    snprintf(buf, bufLen, "%s%s.%d", base, suffix, ind);
  else
    snprintf(buf, bufLen, "%s.%d", base, ind);

  return buf;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::growStep(File &treeFile, File &countFile, 
				   File &outCountFile, File &hCountFile,
				   File &outHCountFile, unsigned maxNodes)
{
  char *line, *tok;
  int rid, rid1;
  Boolean finished = true;

  while (line = countFile.getline()) {
    tok = strtok(line, wordSeparators);
    if (strcmp(tok, "\\begin\\") != 0) continue;
    
    // Initialize
    tok = strtok(0, wordSeparators);
    if (strcmp(tok, "leaf") == 0) continue;

    assert(strcmp(tok, "subtree") == 0);
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &rid) != 1) {
      cerr << "format error in count file\n";
      exit(1);
    }

    if (this->debug(DEBUG_GROW)) 
      this->dout() << "Growing subtree " << rid << "...\n";
    

    this->root = new MultipassNgramDTNode<CountT>(); assert(this->root != 0);
    MROOT->id = rid;
    MROOT->question = new NgramQuestion();
    MROOT->counts = new NgramCounts<CountT>(this->vocab, this->order);
    MROOT->counts->read(countFile);
    MROOT->heldout = new NgramCounts<CountT>(this->vocab, this->order);

    while (line = hCountFile.getline()) {
      tok = strtok(line, wordSeparators);
      if (strcmp(tok, "\\begin\\") != 0) continue;

      tok = strtok(0, wordSeparators);
      if (strcmp(tok, "leaf") == 0) continue;
      assert(strcmp(tok, "subtree") == 0);

      tok = strtok(0, wordSeparators);
      if (sscanf(tok, "%d", &rid1) !=1) {
	cerr << "format error in heldout file\n";
	exit(1);
      }
      if (rid == rid1) break;
      else {
	cerr << "unmatched count and heldout file\n";
	exit(1);
      }
    }
    MROOT->heldout->read(hCountFile);

    if (!growSubtree(rid, treeFile, outCountFile, outHCountFile, maxNodes))
      finished = false;
  }
  
  return finished;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::growSubtree(int rid, File &treeFile, 
				      File &outCountFile, File &outHCountFile, 
				      unsigned maxNodes)
{
  Boolean finished = true;

  Array<MultipassNgramDTNode<CountT> *> leaves;
  int current = 0, offset;
  leaves[0] = MROOT;
  
  // Split the node and compute stats for pruning
  VocabIndex ngram[this->order + 1];
  CountT *count;
  LogP backoffProb;
  
  double gain;
  MultipassNgramDTNode<CountT> *left, *right, *p;
  while (current < leaves.size() && current < maxNodes) {
    p = leaves[current]; assert(p);
    
    // Prepare future counts p->data
    p->data = new ElementType(); assert(p->data != 0);
    NgramCountsIter<CountT> iterC(*(p->counts), ngram, this->order);
    while (count = iterC.next()) {
      Vocab::reverse(ngram);
      *(p->data->future.insert(ngram[0])) += *count; 
      p->data->sum += *count;
	Vocab::reverse(ngram);
    }
    p->data->prepare();
    
    // Compute LL for heldout
    VocabIndex tmp;
    p->LogL = 0.0; p->heldoutSum = 0;
    NgramCountsIter<CountT> iterH(*(p->heldout), ngram, this->order);
    while (count = iterH.next()) {
      Vocab::reverse(ngram);
      
      // Get backoff context
      tmp = ngram[internalOrder-1]; ngram[internalOrder-1] = Vocab_None;
      backoffProb = this->backoffLM->wordProb(ngram[0], &ngram[1]);
      ngram[internalOrder-1] = tmp;
      
      p->LogL += *count * this->wordProb(p->data, ngram[0], backoffProb);
      p->heldoutSum += *count;
      Vocab::reverse(ngram);
    }
    // NOTE: stop splitting when no heldout counts to save computation
    //       correct only when pruning threshold >= 0.0
    if (p->heldoutSum == 0) { p->LogL = 0.0; current++; continue; }
    
    // Split the node
    left = new MultipassNgramDTNode<CountT>(); assert(left);
    right = new MultipassNgramDTNode<CountT>(); assert(right);
    if (this->splitNode(*p, left, right, gain)) {
      p->left = left; p->right = right;
      
      // Pour down heldout data to children
      if (p->left) { 
	MNODE(p->left)->heldout = new NgramCounts<CountT>(this->vocab, 
							  this->order); 
	assert(MNODE(p->left)->heldout);
      }
      if (p->right) {
	MNODE(p->right)->heldout = new NgramCounts<CountT>(this->vocab, 
							   this->order); 
	assert(MNODE(p->right)->heldout);
      }
      
      NgramCountsIter<CountT> iterH(*(p->heldout), ngram, this->order);
      while (count = iterH.next()) {
	Vocab::reverse(ngram);
	if (p->left) {
	  assert(MNODE(p->left)->question);
	  if (MNODE(p->left)->question->isTrue(&ngram[1])) {
	    Vocab::reverse(ngram);
	    *(MNODE(p->left)->heldout->insertCount(ngram)) = *count;
	    Vocab::reverse(ngram);
	  }
	}
	if (p->right) {
	  assert(MNODE(p->right)->question);
	  if (MNODE(p->right)->question->isTrue(&ngram[1])) {
	    Vocab::reverse(ngram);
	    *(MNODE(p->right)->heldout->insertCount(ngram)) = *count;
	    Vocab::reverse(ngram);
	  }
	}
	Vocab::reverse(ngram);
      }
      
      // Save memory; double free safe
      delete p->counts; delete p->heldout; delete p->data;
      p->counts = 0; p->heldout = 0; p->data = 0;
      
      // Enqueue
      offset = leaves.size();
      leaves[offset] = left;  leaves[offset + 1] = right;
    } else {
      delete left; delete right;
    }
    current++;
  }

  if (this->debug(DEBUG_GROW))
    this->dout() << "size = " << leaves.size() 
		 << " current = " << current << endl;

  for (offset = 0; offset < leaves.size(); offset++) 
    if (offset < current) { 
      if (leaves[offset]->isLeaf()) leaves[offset]->id = getLeafId();
    } else {
      leaves[offset]->id = getSubtreeId();
      finished = false;
    }
  
  this->printStats();

  if (this->debug(DEBUG_GROW)) 
    this->dout() << "Dumping subtree " << rid << "...\n";

  dump(&treeFile, &outCountFile, &outHCountFile, rid);
  treeFile.fflush(); outCountFile.fflush();
  //    treeFile.close(); outCountFile.close();
  
  TreeStats t;
  this->treeStats(t, this->root);
  trStats.numNodes += t.numNodes - 1;
  trStats.numLeaves += t.numLeaves - 1;
  
  this->freeTree(MROOT); this->root = 0; //MROOT = 0;

  return finished;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::growSubtreeShallow(int rid, File &treeFile, 
					     unsigned maxNodes)
{
  Boolean finished = true;

  Array<MultipassNgramDTNode<CountT> *> leaves;
  int current = 0, offset;
  leaves[0] = MROOT;
  
  // Split the node 
  VocabIndex ngram[this->order + 1];
  CountT *count;
  LogP backoffProb;
  
  double gain;
  MultipassNgramDTNode<CountT> *left, *right, *p;
  while (current < leaves.size() && current < maxNodes) {
    p = leaves[current]; assert(p);
    
    // Prepare future counts p->data
    p->data = new ElementType(); assert(p->data != 0);
    NgramCountsIter<CountT> iterC(*(p->counts), ngram, this->order);
    while (count = iterC.next()) {
      Vocab::reverse(ngram);
      *(p->data->future.insert(ngram[0])) += *count; 
      p->data->sum += *count;
      Vocab::reverse(ngram);
    }
    p->data->prepare();
    
    // Split the node
    left = new MultipassNgramDTNode<CountT>(); assert(left);
    right = new MultipassNgramDTNode<CountT>(); assert(right);
    if (this->splitNode(*p, left, right, gain)) {
      p->left = left; p->right = right;
      
      // Save memory; double free safe
      delete p->counts; delete p->heldout; delete p->data;
      p->counts = 0; p->heldout = 0; p->data = 0;
      
      // Enqueue
      offset = leaves.size();
      leaves[offset] = left;  leaves[offset + 1] = right;
    } else {
      delete left; delete right;
    }
    current++;
  }

  if (this->debug(DEBUG_GROW))
    this->dout() << "size = " << leaves.size() 
		 << " current = " << current << endl;

  // force any unfinished node to be leaf
  for (offset = 0; offset < leaves.size(); offset++) 
    if (leaves[offset]->isLeaf() || offset >= current)
      leaves[offset]->id = getLeafId();
  
  this->printStats();

  if (this->debug(DEBUG_GROW)) 
    this->dout() << "Dumping subtree " << rid << "...\n";

  dump(&treeFile, 0, 0, rid);
  treeFile.fflush();
  
  TreeStats t;
  this->treeStats(t, this->root);
  trStats.numNodes += t.numNodes - 1;
  trStats.numLeaves += t.numLeaves - 1;
  
  this->freeTree(MROOT); this->root = 0; //MROOT = 0;

  return finished;
}

template <class CountT>
void
MultipassNgramDT<CountT>::grow(unsigned seed)
{
  assert(countFileName); assert(treeFileName); assert(heldoutFileName);

  File *inFile, *outFile, *inFileH, *outFileH, *treeFile;
  
  trStats.numNodes = 1; trStats.numLeaves = 1;

  // Initialize
  if (seed == 0) seed = time(0);
  this->dout() << "seed= " << seed << endl;
  srand(seed);
  numSubtrees = 0; numLeaves = 0; // reset
  int rid = getSubtreeId();
  
  if (this->debug(DEBUG_GROW))
    this->dout() << "Growing subtree " << rid << "...\n";

  this->root = new MultipassNgramDTNode<CountT>(); assert(this->root != 0);
  MROOT->id = rid;
  MROOT->question = new NgramQuestion();

  /* Save memory and one (huge) pass of I/O */
  MROOT->counts = this->pcnts; this->pcnts = 0;
  MROOT->heldout = this->heldout; this->heldout = 0;

  treeFile = new File(getFN(treeFileName, 0 ,".full"), "w"); assert(treeFile);
  outFile = new File(getFN(countFileName, 1), "w"); assert(outFile);
  outFileH = new File(getFN(heldoutFileName, 1), "w"); assert(outFileH);

  Boolean finished = growSubtree(rid, *treeFile, *outFile, *outFileH,maxNodes);

  delete outFile; delete outFileH;

  unsigned i = 1;
  while (!finished) {
    inFile = new File(getFN(countFileName, i), "r"); assert(inFile);
    outFile = new File(getFN(countFileName, i+1), "w"); assert(outFile);
    inFileH = new File(getFN(heldoutFileName, i), "r"); assert(inFileH);
    outFileH = new File(getFN(heldoutFileName, i+1), "w"); assert(outFileH);
  
    treeFile = new File(getFN(treeFileName,i,".full"), "w"); assert(treeFile);

    finished = growStep(*treeFile, *inFile, *outFile, *inFileH, *outFileH,
			maxNodes*(i+1));

    delete treeFile;
    delete inFile; delete outFile;
    delete inFileH; delete outFileH;

    i++;
  }

  numSteps = i;

  if (this->debug(DEBUG_GROW))
    this->dout() << "Full tree numNodes: " << trStats.numNodes
		 << ", numLeaves: " << trStats.numLeaves << endl;
}

template <class CountT>
void
MultipassNgramDT<CountT>::growShallow(unsigned seed)
{
  assert(treeFileName); 

  File *treeFile;
  
  trStats.numNodes = 1; trStats.numLeaves = 1;

  // Initialize
  if (seed == 0) seed = time(0);
  this->dout() << "seed= " << seed << endl;
  srand(seed);
  numSubtrees = 0; numLeaves = 0; // reset
  int rid = getSubtreeId();
  
  if (this->debug(DEBUG_GROW))
    this->dout() << "Growing subtree " << rid << "...\n";

  this->root = new MultipassNgramDTNode<CountT>(); assert(this->root != 0);
  MROOT->id = rid;
  MROOT->question = new NgramQuestion();

  /* Save memory and one (huge) pass of I/O */
  MROOT->counts = this->pcnts; this->pcnts = 0;

  treeFile = new File(getFN(treeFileName, 0), "w"); assert(treeFile);

  Boolean finished = growSubtreeShallow(rid, *treeFile, maxNodes);
  
  numSteps = 1;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::dump(File *treeFile, File *countFile, 
			       File *hCountFile, int rootId,
			       IdSetType *idList)
{
  Boolean written = false;

  if (MROOT == 0) return written;
  assert(rootId < 0);

  if (treeFile) treeFile->fprintf("\\begin\\\n");
  if (rootId != MROOT->id) {
    assert(MROOT->isLeaf()); assert(MROOT->id > 0);
    if (treeFile)
      treeFile->fprintf("subtree %d = leaf %d %f %d\n", rootId, MROOT->id, 
	      MROOT->LogL, MROOT->heldoutSum);
    
  } else 
    written = dumpTree(treeFile, countFile, hCountFile, MROOT, idList);
  if (treeFile) treeFile->fprintf("\\end\\\n");

  return written;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::dumpTree(File *treeFile, File *countFile, 
				   File *hCountFile,
				   MultipassNgramDTNode<CountT> *p,
				   IdSetType *idList)
{
  Boolean written = false;

  if (p == 0) {
    if (treeFile) treeFile->fprintf("null\n");
    return written;
  }
  
  if (treeFile) {
    if (p->id < 0)
      treeFile->fprintf("subtree %d %f %d\n", p->id, p->LogL, p->heldoutSum);
    else if (p->id > 0)
      treeFile->fprintf("leaf %d %f %d\n", p->id, p->LogL, p->heldoutSum);
    else 
      treeFile->fprintf("node %f %d\n", p->LogL, p->heldoutSum);
  }
  
  if (treeFile) {
    assert(p->question != 0);
    QUESTION(p->question)->writeText(*treeFile, this->vocab);
  }

  if (p->isLeaf()) {
    assert(p->id != 0);

    if (p->id < 0) {
      assert(p != MROOT);
      if (idList) idList->insert(p->id);

      if (countFile && p->counts != 0) {
	countFile->fprintf("\\begin\\ subtree %d\n", p->id);
	p->counts->write(*countFile);
	countFile->fprintf("\\end\\\n");
      }
      if (hCountFile && p->heldout != 0) {
	hCountFile->fprintf("\\begin\\ subtree %d\n", p->id);
	p->heldout->write(*hCountFile); written = true;
	hCountFile->fprintf("\\end\\\n");
      }
    }
  } else {
    Boolean w1 = dumpTree(treeFile, countFile, hCountFile, MNODE(p->left), 
			  idList);
    Boolean w2 = dumpTree(treeFile, countFile, hCountFile, MNODE(p->right),
			  idList);

    written = (w1 || w2);
  }

  return written;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::load(File &treeFile, File *pCountFile, int &rootId,
			       Boolean loadByOrder)
{
  char *line = treeFile.getline();
  if (line == 0) { 
    this->root = 0; /*MROOT = 0;*/ 
    if (loadByOrder) rootId = 0; // unknown root id
    return true; 
  }

  if (strncmp(line, "\\end\\", 5) == 0) line = treeFile.getline();  

  if (line == 0) {
    this->root = 0; if(loadByOrder) rootId = 0;
    return true;
  }

  char *tok = strtok(line, wordSeparators);
  while (strcmp(tok, "\\begin\\") != 0) {
    line = treeFile.getline();
    if (line == 0) return false;
    tok = strtok(line, wordSeparators);
  }
  
  line = treeFile.getline(); 
  if (line == 0) return false;
  tok = strtok(line, wordSeparators);
  if (strcmp(tok, "\\end\\") == 0) { 
    this->root = 0; /*MROOT = 0;*/ 
    if (loadByOrder) rootId = 0;
    return true; 
  }
  assert(strcmp(tok, "subtree") == 0);

  // Load the root
  this->root = new MultipassNgramDTNode<CountT>(); assert(MROOT);
  tok = strtok(0, wordSeparators);
  if (sscanf(tok, "%d", &MROOT->id) != 1 || MROOT->id >= 0) { // subtree id < 0
    delete MROOT; return false; 
  }  
  
  if (loadByOrder && MROOT->id != rootId) {
    assert(rootId < 0);

    // Skip this tree
    do {
      line = treeFile.getline();
    } while (line != 0 && strncmp(line, "\\end\\", 5) != 0);
    
    rootId = MROOT->id; delete MROOT;
    return true;
  }

  if (this->debug(DEBUG_GROW)) this->dout() << "root->id=" << MROOT->id << endl;

  tok = strtok(0, wordSeparators);
  if (strcmp(tok, "=") == 0) {
    tok = strtok(0, wordSeparators); assert(strcmp(tok, "leaf") == 0);
    
    // Read alias leaf id
    rootId = MROOT->id;
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &MROOT->id) != 1 || MROOT->id <= 0) { // leaf id > 0
      delete MROOT; return false; 
    }
    
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%f", &MROOT->LogL) != 1) { delete MROOT; return false; }
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &MROOT->heldoutSum)!=1) {delete MROOT; return false;}
    
    MROOT->question = new NgramQuestion(); assert(MROOT->question);
    line = treeFile.getline(); // consume end of tree symbol

    return true;
  } else {
    rootId = MROOT->id; // no alias id

    if (sscanf(tok, "%f", &MROOT->LogL) != 1) { delete MROOT; return false; }
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &MROOT->heldoutSum)!=1) {delete MROOT; return false;}
    MROOT->question = new NgramQuestion(); assert(MROOT->question);
    if (!QUESTION(MROOT->question)->readText(treeFile, this->vocab)) {
      delete MROOT; return false;
    }

    // Call loadTree() to load the rest
    Boolean ok;
    MROOT->left = loadTree(treeFile, pCountFile, ok);
    if (ok) MROOT->right = loadTree(treeFile, pCountFile, ok);
    
    return ok;
  }
}

template <class CountT>
MultipassNgramDTNode<CountT> *
MultipassNgramDT<CountT>::loadTree(File &treeFile, File *pCountFile, 
				   Boolean &ok)
{
  char *line = treeFile.getline(); 
  if (line == 0) { ok = true; return 0; }
  char *tok = strtok(line, wordSeparators);
  if (strcmp(tok, "\\end\\") == 0) { ok = true; return 0; }

  MultipassNgramDTNode<CountT> *p = 0;
  if (strcmp(tok, "node") == 0) {
    p = new MultipassNgramDTNode<CountT>(); assert(p);

    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%f", &p->LogL) != 1) { ok = false; goto stop; }
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &p->heldoutSum) != 1) { ok = false; goto stop; }

    p->question = new NgramQuestion(); assert(p->question);
    ok = QUESTION(p->question)->readText(treeFile, this->vocab);

    if (ok) {
      p->left = loadTree(treeFile, pCountFile, ok);
      if (ok) p->right = loadTree(treeFile, pCountFile, ok);
    }
  } else if (strcmp(tok, "leaf") == 0) {
    p = new MultipassNgramDTNode<CountT>(); assert(p);

    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &p->id) != 1 || p->id <= 0) { // leaf id > 0
      ok = false; goto stop;
    } 
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%f", &p->LogL) != 1) { ok = false; goto stop; }

    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &p->heldoutSum) != 1) { ok = false; goto stop; }
    
    p->question = new NgramQuestion(); assert(p->question);
    ok = QUESTION(p->question)->readText(treeFile, this->vocab);

  } else if (strcmp(tok, "subtree") == 0) {
    p = new MultipassNgramDTNode<CountT>(); assert(p);
     
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &p->id) != 1 || p->id >= 0) { // subtree id < 0
      ok = false; goto stop;
    }
    
    p->question = new NgramQuestion(); assert(p->question);
    ok = QUESTION(p->question)->readText(treeFile, this->vocab);
  } else if (strcmp(tok, "null") == 0) {
    ok = true;
  } else
    ok = false;

  stop: 
  if (ok) return p;
  else { delete p; return 0; }
}

template <class CountT>
void
MultipassNgramDT<CountT>::prune(const double threshold)
{
  assert(this->discount); assert(this->backoffLM); //assert(this->heldout);

  TreeStats tstats;

  slist = new StatsList(); assert(slist);
  File *treeFile, *outFile;
  int i, rootId;

  for (i=numSteps-1; i>=0; i--) {
    treeFile = new File(getFN(treeFileName, i, ".full"), "r"); 
    assert(treeFile);
    outFile = new File(getFN(treeFileName, i), "w"); assert(outFile);

    while (treeFile->feof() == 0) {
      // Read in tree
      if (!load(*treeFile, 0, rootId)) {
	cerr << "format error in tree file: " << treeFile->lineno << endl;
	exit(1);
      }

      // Compute potential and prune tree
      LogP LogL;
      CountT heldoutSum;
      compPtnlAndPrune(MROOT, threshold, LogL, heldoutSum);

      // Save stats
      if (MROOT) {
	PruneStatsType *pstats = slist->insert(rootId); assert(pstats);
	pstats->leavesLikelihood = LogL;
	pstats->leavesSum = heldoutSum;
      }
      
      // Fix node id
      postPruneProcess(rootId);

      // Dump pruned tree
      dump(outFile, 0, 0, rootId);

      // Free memory
      this->freeTree(MROOT); this->root = 0; // MROOT = 0;
    }
    
    delete outFile;
    delete treeFile;
  }
  
  delete slist;
}

template <class CountT>
void
MultipassNgramDT<CountT>::compPtnlAndPrune(MultipassNgramDTNode<CountT> *p,
					   double threshold,
					   LogP &leavesLikelihood,
					   CountT &leavesSum)
{
  if (p == 0) { leavesLikelihood = 0.0; leavesSum = 0; return; }

  if (p->isLeaf()) {
    if (p->id > 0) { // leaf node
      p->potential = 0;

      leavesLikelihood = p->LogL;
      leavesSum = p->heldoutSum;
    } else { // subtree node
      assert(p->id < 0);

      // Find stats in slist
      PruneStatsType *pstats = slist->find(p->id);
      if (pstats == 0) {
	cerr << "missing stats in tree file: " << p->id << endl;
	exit(1);
      }
      leavesLikelihood = pstats->leavesLikelihood;
      leavesSum = pstats->leavesSum;
    }

    return;
  }

  LogP leftLogL, rightLogL;
  CountT leftSum, rightSum;
  compPtnlAndPrune(MNODE(p->left), threshold, leftLogL, leftSum);
  compPtnlAndPrune(MNODE(p->right), threshold, rightLogL, rightSum);

  // Return statistics
  leavesLikelihood = leftLogL + rightLogL;
  leavesSum = leftSum + rightSum;
  
  // Compute potential
  p->potential = ((leavesSum==0) ? 0 : leavesLikelihood/leavesSum) 
    - ((p->heldoutSum==0)? 0 : p->LogL/p->heldoutSum);

  if (p->potential <= threshold) {
  
    /* NOTE: Because thoeretically p->potential can take values 
       as small as Prob_Epsilon , there is NO way to garantee this pruning
       procedure gives exactly the same result as NgramDecisionTree::prune()
       even though it should!!!

       The only way to get around this might be to use double instead of 
       float for lprob. But since SRI LM Toolkit is using float for lprob
       anyway, I decide to settle for float as well.
    */

    this->freeTree(p->left); p->left = 0;
    this->freeTree(p->right); p->right = 0;
  }
}

template <class CountT>
void
MultipassNgramDT<CountT>::postPruneProcess(int &rootId)
{
  if (MROOT == 0) return;

  if (MROOT->isLeaf() && MROOT->id < 0) {
    rootId = MROOT->id;
    MROOT->id = getLeafId();
  }
  postPruneTree(MNODE(MROOT->left));
  postPruneTree(MNODE(MROOT->right));
}

template <class CountT>
void
MultipassNgramDT<CountT>::postPruneTree(MultipassNgramDTNode<CountT> *p)
{
  if (p == 0) return;
  
  if (p->isLeaf() && p->id == 0)
    p->id = getLeafId();

  postPruneTree(MNODE(p->left));
  postPruneTree(MNODE(p->right));
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::read(File &file)
{
  cerr << "MultipassNgramDT<CountT>::read(File &file) doesn't exist!\n";
  return true;
}

template <class CountT>
void
MultipassNgramDT<CountT>::write(File &file)
{
  File *treeFile;
  int i, j, k, rootId, id;
  Boolean finished = false;
  IdSetType idList;

  idList.insert(-1);
  i=0;
  while (!finished && i<numSteps) {
    treeFile = new File(getFN(treeFileName, i), "r"); assert(treeFile);
    
    while (treeFile->feof() == 0) {
      if (!load(*treeFile, 0, rootId)) {
	cerr << "format error in tree file: " << treeFile->lineno << endl;
	exit(1);
      }
      if (MROOT == 0) continue;
      
      // Check if this subtree is needed
      if (idList.find(rootId)) {
	dump(&file, 0, 0, rootId, &idList);

	idList.remove(rootId);
      }
      
      this->freeTree(MROOT); this->root = 0;
    }

    delete treeFile;
    i++;
    
    finished = (idList.numEntries() == 0);
  }
}

template <class CountT>
void
MultipassNgramDT<CountT>::eval(NgramCounts<CountT> &testStats,
			       Ngram &lm)
{
  assert(countFileName); assert(testFileName); assert(treeFileName);
  
  File *inFile, *outFile, *inFileT, *outFileT;
  File treeFile(treeFileName, "r");

  // Initialize
  int rid = -1;
  if (!load(treeFile, 0, rid, true)) {
    cerr << "format error in tree file: " << treeFile.lineno << endl;
    exit(1);
  }

  if (rid != -1) {
    cerr << "rootId = " << rid << endl;
    cerr << "unmatched tree file and count file\n";
    exit(1);
  }
  
  /* Save memory and one pass of I/O */
  MROOT->counts = this->pcnts; this->pcnts = 0;
  /* But we don't want to destroy testStats */
  MROOT->heldout = new NgramCounts<CountT>(this->vocab, this->order);

  unsigned tt = this->cloneCounts(*(MROOT->heldout), testStats);
  if (this->debug(DEBUG_GROW)) this->dout() << tt << " ngrams cloned.\n";

  outFile = new File(getFN(countFileName, 1), "w"); assert(outFile);
  outFileT = new File(getFN(testFileName, 1), "w"); assert(outFileT);

  Boolean finished = !evalSubtree(rid, *outFile, *outFileT, lm);

  delete outFile; delete outFileT;

  unsigned i = 1;
  while (!finished) {
    inFile = new File(getFN(countFileName, i), "r"); assert(inFile);
    outFile = new File(getFN(countFileName, i+1), "w"); assert(outFile);
    inFileT = new File(getFN(testFileName, i), "r"); assert(inFileT);
    outFileT = new File(getFN(testFileName, i+1), "w"); assert(outFileT);

    finished = evalStep(treeFile, *inFile, *outFile, *inFileT, *outFileT, lm);

    delete inFile; delete outFile;
    delete inFileT; delete outFileT;

    i++;
  }
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::evalStep(File &treeFile, File &countFile,
				   File &outCountFile, File &tCountFile,
				   File &outTCountFile, Ngram &lm)
{
  char *line, *tok;
  int rid, rid1;
  Boolean finished = true, written;
  
  while (line = countFile.getline()) {
    tok = strtok(line, wordSeparators);
    if (strcmp(tok, "\\begin\\") != 0) continue;
    
    // Initialize
    tok = strtok(0, wordSeparators);
    if (strcmp(tok, "leaf") == 0) continue;
    
    assert(strcmp(tok, "subtree") == 0);
    tok = strtok(0, wordSeparators);
    if (sscanf(tok, "%d", &rid) != 1) {
      cerr << "format error in count file\n";
      exit(1);
    }
    
    if (this->debug(DEBUG_GROW)) 
      this->dout() << "Processing subtree " << rid << "...\n";

    // Read in a specific tree 
    int rootId;
    do {
      rootId = rid;
      
      if (!load(treeFile, 0, rootId, true)) {
	cerr << "format error in tree file: " << treeFile.lineno << endl;
	exit(1);
      }
      
      if (this->debug(DEBUG_GROW))
	this->dout() << "Encountered tree " << rootId << endl;
      
    } while (rootId != 0 && rootId != rid);
    
    if (rootId != rid) {
      cerr << "rootId = " << rootId << " rid= " << rid << endl;
      cerr << "unmatched tree file and count file\n";
      exit(1);
    }
    
    /* Note: From now on, we use "node->heldout" to accommandate test counts;
       read it as "node->test" or "node->secondary". */
    
    // Read in counts
    MROOT->counts = new NgramCounts<CountT>(this->vocab, this->order);
    assert(MROOT->counts);
    MROOT->counts->read(countFile);
    
    MROOT->heldout = new NgramCounts<CountT>(this->vocab, this->order);
    assert(MROOT->heldout);

    while (line = tCountFile.getline()) {
      tok = strtok(line, wordSeparators);
      if (strcmp(tok, "\\begin\\") != 0) continue;

      tok = strtok(0, wordSeparators);
      if (strcmp(tok, "leaf") == 0) continue;
      assert(strcmp(tok, "subtree") == 0);

      tok = strtok(0, wordSeparators);
      if (sscanf(tok, "%d", &rid1) !=1) {
        cerr << "format error in heldout file\n";
        exit(1);
      }
      if (rid == rid1) break;
      else {
        cerr << "unmatched count and heldout file\n";
        exit(1);
      }
    }
    MROOT->heldout->read(tCountFile);

    written = evalSubtree(rid, outCountFile, outTCountFile, lm);

    finished = finished && !written;
  }
  
  return finished;
}

template <class CountT>
Boolean
MultipassNgramDT<CountT>::evalSubtree(int rid, File &outCountFile, 
				      File &outTCountFile, Ngram &lm)
{
  if (this->debug(DEBUG_GROW)) this->dout() << "Pouring down the counts...\n";
  // Pour down counts 
  loadAndPourCounts();

  if (this->debug(DEBUG_GROW)) this->dout() << "Computing probs...\n";
  // Compute probs for test ngrams and save them to lm
  unsigned t = computeProbTree(MROOT, lm);
  if (this->debug(DEBUG_GROW)) this->dout() << t << " ngrams computed\n";

  // Dump counts for the next iteration
  Boolean written = dump(0, &outCountFile, &outTCountFile, rid);

  // Free tree
  this->freeTree(MROOT); this->root = 0;  

  return written;
}

template <class CountT>
void
MultipassNgramDT<CountT>::loadAndPourCounts()
{
  // Note: do two things together to save computation
  assert(MROOT); assert(MROOT->counts); assert(MROOT->heldout);

  Boolean foundP;
  CountT *count;
  VocabIndex ngram[this->order+1];

  // Pour down test counts first so we can save computation in next step
  MultipassNgramDTNode<CountT> *p;
  NgramCountsIter<CountT> iter(*MROOT->heldout, ngram, this->order);
  while (count = iter.next()) {
    Vocab::reverse(ngram);
    p = MNODE(this->find1(&ngram[1], foundP));
    
    Vocab::reverse(ngram);
    if (foundP) {
      if (p->heldout == 0) 
	p->heldout = new NgramCounts<CountT>(this->vocab, this->order);
      assert(p->heldout);
      *(p->heldout->insertCount(ngram)) = *count;

      if (this->dumpfile != 0 && p->id > 0) { 
	this->dumpfile->fprintf("%d ", p->id);
	this->dumpfile->fprintf("%s", this->vocab.getWord(ngram[0]));
	for(unsigned i=1; i<this->order; i++)
	  this->dumpfile->fprintf(" %s", this->vocab.getWord(ngram[i]));
	this->dumpfile->fprintf("\n");
      }
    }
  }
  
  if (this->debug(DEBUG_GROW)) 
    this->dout() << "order = " << this->order << endl;

  // Load (for leaf) and pour down (for subtree) counts on demand
  NgramCountsIter<CountT> iter1(*MROOT->counts, ngram, this->order);  
  while (count = iter1.next()) {
    Vocab::reverse(ngram);
    p = MNODE(this->find1(&ngram[1], foundP));

    if (foundP && p->heldout != 0 && p->id > 0) {
      if (p->data == 0) p->data = new ElementType();
      assert(p->data);
      *(p->data->future.insert(ngram[0])) += *count;
      p->data->sum += *count;
    }

    Vocab::reverse(ngram);

    if (foundP && p->heldout != 0 && p->id < 0) {
      if (p->counts == 0)
	p->counts = new NgramCounts<CountT>(this->vocab, this->order);
      assert(p->counts);
      *(p->counts->insertCount(ngram)) = *count;
    }

    if (foundP && this->dumpfile != 0 && p->id > 0) {
      this->dumpfile->fprintf("%d ", p->id);
      this->dumpfile->fprintf("%s", this->vocab.getWord(ngram[0]));
      for(unsigned i=1; i<this->order; i++)
	this->dumpfile->fprintf(" %s", this->vocab.getWord(ngram[i]));
      this->dumpfile->fprintf("\n");
    }
  }
  
  this->prepareCounts();
}

template <class CountT>
unsigned
MultipassNgramDT<CountT>::computeProbTree(MultipassNgramDTNode<CountT> *p,
					  Ngram &lm)
{
  if (p == 0) return 0;

  if (p->isLeaf() && p->id > 0 && p->heldout != 0) {
    //assert(p->data);
    if (p->data == 0) {
      cerr << "p->data == 0 can't continue.\n";
      if (p->counts == 0) cerr << "p->counts == 0 too.\n"; 
      cerr << "p->id = " << p->id << endl;
      exit(1);
    }
    
    VocabIndex tmp;
    VocabIndex ngram[this->order + 1];
    CountT *count;
    LogP backoffProb, lprob;
    unsigned w = 0;
    int i;

    NgramCountsIter<CountT> iter(*(p->heldout), ngram, this->order);
    while (count = iter.next()) {
      Vocab::reverse(ngram);
      
      // Get backoff context
      tmp = ngram[internalOrder-1]; ngram[internalOrder-1] = Vocab_None;
      backoffProb = this->backoffLM->wordProb(ngram[0], &ngram[1]);
      ngram[internalOrder-1] = tmp;
      
      lprob = this->wordProb(p->data, ngram[0], backoffProb);
      if (this->infoDumpFile != 0) { // Output detailed info for ngram comp.
	assert(this->getSeenHistory());
	if (this->getSeenFuture()) {
	  this->infoDumpFile->fprintf("seen");
	} else {
	  this->infoDumpFile->fprintf("unseen_seen");
	}
	for (i=this->order-1; i>=0; i--)
	  this->infoDumpFile->fprintf(" %s", this->vocab.getWord(ngram[i]));
	this->infoDumpFile->fprintf(" %e\n", LogPtoProb(lprob));
      }

      // Save prob in lm
      *lm.insertProb(ngram[0], &ngram[1]) = lprob; w++;
      Vocab::reverse(ngram);
    }
    
    return w;
  } else {
    unsigned w1 = computeProbTree(MNODE(p->left), lm);
    unsigned w2 = computeProbTree(MNODE(p->right), lm);

    return w1+w2;
  }
}
