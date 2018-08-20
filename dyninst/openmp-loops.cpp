//
//  Copyright (c) 2017-2018, Rice University.
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
//  * Neither the name of Rice University (RICE) nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
//  This software is provided by RICE and contributors "as is" and any
//  express or implied warranties, including, but not limited to, the
//  implied warranties of merchantability and fitness for a particular
//  purpose are disclaimed. In no event shall RICE or contributors be
//  liable for any direct, indirect, incidental, special, exemplary, or
//  consequential damages (including, but not limited to, procurement of
//  substitute goods or services; loss of use, data, or profits; or
//  business interruption) however caused and on any theory of liability,
//  whether in contract, strict liability, or tort (including negligence
//  or otherwise) arising in any way out of the use of this software, even
//  if advised of the possibility of such damage.
//
// ----------------------------------------------------------------------
//
//  This program is a proxy for the function and loop hierarchy parts
//  of hpcstruct with openmp threads.
//
//  This test creates a ParseAPI::CodeObject and runs parse() in
//  parallel.  Then, it sequentially iterates through the function and
//  loop hierarchy and prints the loop tree and basic blocks of every
//  function and loop.  Everything is sorted by VMA address, so the
//  output should be deterministic.
//
//  This program tests if ParseAPI produces deterministic results for
//  functions, loops and blocks.
//
//  Build me as a Dyninst application with -fopenmp, or else use
//  scripts from github.com/mwkrentel/myrepo.
//
//  Usage:
//  ./openmp-loops  [options] ...  filename
//
//  Options:
//   -j, -jp num  use <num> threads for ParseAPI::parse()
//   -js num      use <num> threads for Symtab methods
//   -p, -v       print verbose function information
//   -h, --help   display usage message and exit
//

#define MY_USE_OPENMP  1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if MY_USE_OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <mutex>

#include <CFG.h>
#include <CodeObject.h>
#include <CodeSource.h>
#include <Function.h>
#include <Symtab.h>
#include <Instruction.h>
#include <LineInformation.h>

#define MAX_VMA  0xfffffffffffffff0

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;
using namespace std;

typedef unsigned long VMA;
typedef unsigned int uint;

typedef vector <Block *> BlockVec;
typedef map <Block *, bool> BlockSet;
typedef map <VMA, VMA> RangeSet;

Symtab * the_symtab = NULL;

// Command-line options
class Options {
public:
    const char *filename;
    int   jobs_parse;
    int   jobs_symtab;
    bool  verbose;

  Options() {
	filename = NULL;
	jobs_parse = 1;
	jobs_symtab = 1;
	verbose = false;
    }
};

Options opts;

//----------------------------------------------------------------------

// Sort Blocks by start address, low to high.
static bool
BlockLessThan(Block * b1, Block * b2)
{
    return b1->start() < b2->start();
}

// Sort Edges first by target address (usually all the same), and then
// by source address.
static bool
EdgeLessThan(Edge * e1, Edge * e2)
{
    return (e1->trg()->start() < e2->trg()->start())
	|| (e1->trg()->start() == e2->trg()->start()
	    && e1->src()->last() < e2->src()->last());
}

// Sort Functions by entry address, low to high.
static bool
FuncLessThan(ParseAPI::Function * f1, ParseAPI::Function * f2)
{
    return f1->addr() < f2->addr();
}

//----------------------------------------------------------------------

// Returns: the min entry VMA for the loop, or else 0 if the loop is
// somehow invalid.  Irreducible loops have more than one entry
// address.
static VMA
LoopMinEntryAddr(Loop * loop)
{
    if (loop == NULL) {
	return 0;
    }

    vector <Block *> entBlocks;
    int num_ents = loop->getLoopEntries(entBlocks);

    if (num_ents < 1) {
	return 0;
    }

    VMA ans = MAX_VMA;
    for (int i = 0; i < num_ents; i++) {
	ans = std::min(ans, entBlocks[i]->start());
    }

    return ans;
}

// Sort Loops (from their LoopTreeNodes) by min entry VMAs.
static bool
LoopTreeLessThan(LoopTreeNode * n1, LoopTreeNode * n2)
{
    return LoopMinEntryAddr(n1->loop) < LoopMinEntryAddr(n2->loop);
}

//----------------------------------------------------------------------

void
printBlocks(BlockVec & bvec)
{
    for (auto bit = bvec.begin(); bit != bvec.end(); ++bit) {
	VMA start = (*bit)->start();
	VMA end = (*bit)->end();

	cout << "0x" << hex << start << "--0x" << end << dec
	     << "  (" << end - start << ")\n";
    }
}

//----------------------------------------------------------------------

void
printRangeSet(RangeSet & rset)
{
    int max_per_line = 4;
    int num_on_line = 0;

    cout << hex;

    for (auto rit = rset.begin(); rit != rset.end(); ++rit) {
	if (num_on_line > 0) { cout << "  "; }

	cout << "0x" << rit->first << "--0x" << rit->second;
	num_on_line++;

	if (num_on_line == max_per_line) {
	    cout << "\n";
	    num_on_line = 0;
	}
    }
    if (num_on_line > 0) { cout << "\n"; }

    cout << dec;
}

//----------------------------------------------------------------------

// Add one range to the range set (no attributes).
void
addRange(RangeSet & rset, VMA start, VMA end)
{
    // empty range
    if (start >= end) {
	return;
    }

    // it = first range strictly right of start, or else end
    // left = range containing start, if there is one
    auto it = rset.upper_bound(start);
    auto left = it;
    if (left != rset.begin()) {
	--left;
    }

    if (left != rset.end() && left->first <= start && start <= left->second) {
	// new range overlaps with left, merge
	start = left->first;
	end = std::max(end, left->second);
	rset.erase(left);
    }

    // delete any ranges contained within the new one
    while (it != rset.end() && it->second <= end) {
	auto next = it;  ++next;
	rset.erase(it);
	it = next;
    }

    if (it != rset.end() && it->first <= end) {
	// new range overlaps with 'it', merge
	end = std::max(end, it->second);
	rset.erase(it);
    }

    // add combined range [start, end)
    rset[start] = end;
}

//----------------------------------------------------------------------

void
doLoop(Loop * loop, BlockSet & visited, string name)
{
    // entry blocks
    BlockVec entBlocks;
    int num_ents = loop->getLoopEntries(entBlocks);

    cout << "\n----------------------------------------\n"
	 << "loop:  0x" << hex << LoopMinEntryAddr(loop) << dec
	 << "  " << ((num_ents == 1) ? "(reducible)" : "(irreducible)")
	 << "  " << name << "\n";

    std::sort(entBlocks.begin(), entBlocks.end(), BlockLessThan);

    cout << "\nentry blocks:" << hex;
    for (auto bit = entBlocks.begin(); bit != entBlocks.end(); ++bit) {
	cout << "  0x" << (*bit)->start();
    }
    cout << "\n";

    // back edges
    vector <Edge *> backEdges;
    loop->getBackEdges(backEdges);

    std::sort(backEdges.begin(), backEdges.end(), EdgeLessThan);

    cout << "\nback edge sources:";
    for (auto eit = backEdges.begin(); eit != backEdges.end(); ++eit) {
	cout << "  0x" << (*eit)->src()->last();
    }

    cout << "\nback edge targets:";
    for (auto eit = backEdges.begin(); eit != backEdges.end(); ++eit) {
	cout << "  0x" << (*eit)->trg()->start();
    }
    cout << dec << "\n";

    // inclusive and exclusive blocks and ranges
    BlockVec inclBlocks;
    BlockVec exclBlocks;
    RangeSet inclRange;
    RangeSet exclRange;

    loop->getLoopBasicBlocks(inclBlocks);

    for (auto bit = inclBlocks.begin(); bit != inclBlocks.end(); ++bit) {
	Block * block = *bit;

	if (! visited[block]) {
	    exclBlocks.push_back(block);
	    addRange(exclRange, block->start(), block->end());
	}
	addRange(inclRange, block->start(), block->end());
	visited[block] = true;
    }

    cout << "\ninclusive range:\n";
    printRangeSet(inclRange);

    cout << "\nexclusive range:\n";
    printRangeSet(exclRange);

    std::sort(exclBlocks.begin(), exclBlocks.end(), BlockLessThan);

    cout << "\nloop blocks:\n";
    printBlocks(exclBlocks);
}

//----------------------------------------------------------------------

void
doLoopTree(LoopTreeNode * ltnode, BlockSet & visited, string name,
	   bool summary)
{
    // recur on subloops
    vector <LoopTreeNode *> clist = ltnode->children;

    std::sort(clist.begin(), clist.end(), LoopTreeLessThan);

    for (uint i = 0; i < clist.size(); i++) {
        doLoopTree(clist[i], visited, name + "-" + to_string(i + 1), summary);
    }

    if (summary) {
	// summary of loop tree for func header
	BlockVec entBlocks;
	Loop * loop = ltnode->loop;
	int num_ents = loop->getLoopEntries(entBlocks);

	cout << "0x" << hex << LoopMinEntryAddr(loop) << dec
	     << " " << ((num_ents == 1) ? " (reducible) " : "(irreducible)")
	     << " " << name << "\n";
    } else {
	// this loop last, in post order
	doLoop(ltnode->loop, visited, name);
    }
}

//----------------------------------------------------------------------

void
doFunction(ParseAPI::Function * func)
{
    cout << "\n==================================================\n"
	 << "func:  0x" << hex << func->addr() << dec
	 << "  " << func->name() << "\n";

    // inclusive blocks and range
    const ParseAPI::Function::blocklist & blist = func->blocks();
    BlockSet visited;
    RangeSet inclRange;

    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
        Block * block = *bit;
        visited[block] = false;
	addRange(inclRange, block->start(), block->end());
    }

    cout << "\ninclusive range:\n";
    printRangeSet(inclRange);

    LoopTreeNode * ltnode = func->getLoopTree();
    vector <LoopTreeNode *> clist = ltnode->children;

    std::sort(clist.begin(), clist.end(), LoopTreeLessThan);

    // summary of loop tree
    if (clist.size() > 0) {
	cout << "\nloop tree:\n";
	for (uint i = 0; i < clist.size(); i++) {
	    doLoopTree(clist[i], visited, "loop-" + to_string(i + 1), true);
	}
    }

    // visit loops, there is no top-level loop, only children
    for (uint i = 0; i < clist.size(); i++) {
        doLoopTree(clist[i], visited, "loop-" + to_string(i + 1), false);
    }

    // exclusive blocks and range
    BlockVec exclBlocks;
    RangeSet exclRange;

    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
        Block * block = *bit;

	if (! visited[block]) {
	    exclBlocks.push_back(block);
	    addRange(exclRange, block->start(), block->end());
	}
    }

    if (clist.size() > 0) {
	cout << "\n-----------------------------------\n"
	     << "end func:  0x" << hex << func->addr() << dec
	     << "  " << func->name() << "\n";
	cout << "\ninclusive range:\n";
	printRangeSet(inclRange);
    }

    cout << "\nexclusive range:\n";
    printRangeSet(exclRange);

    std::sort(exclBlocks.begin(), exclBlocks.end(), BlockLessThan);

    cout << "\nfunc blocks:\n";
    printBlocks(exclBlocks);
}

//----------------------------------------------------------------------

void
usage(string mesg)
{
    if (! mesg.empty()) {
	cout << "error: " << mesg << "\n\n";
    }

    cout << "usage:  openmp-parse  [options]...  filename\n\n"
	 << "options:\n"
	 << "  -j, -jp num  use num threads for ParseAPI::parse()\n"
	 << "  -js num      use num threads for SymtabAPI\n"
	 << "  -p, -v       print verbose function information\n"
	 << "  -h, --help   display usage message and exit\n"
	 << "\n";

    exit(1);
}

// Command-line:  [options] ...  filename
void
getOptions(int argc, char **argv, Options & opts)
{
    if (argc < 2) {
	usage("");
    }

    int n = 1;
    while (n < argc) {
	string arg(argv[n]);

	if (arg == "-h" || arg == "-help" || arg == "--help") {
	    usage("");
	}
	else if (arg == "-j" || arg == "-jp") {
	    if (n + 1 >= argc) {
	        usage("missing arg for -jp");
	    }
	    opts.jobs_parse = atoi(argv[n + 1]);
	    if (opts.jobs_parse <= 0) {
	        errx(1, "bad arg for -jp: %s", argv[n + 1]);
	    }
	    n += 2;
	}
	else if (arg == "-js") {
	    if (n + 1 >= argc) {
	        usage("missing arg for -js");
	    }
	    opts.jobs_symtab = atoi(argv[n + 1]);
	    if (opts.jobs_symtab <= 0) {
	        errx(1, "bad arg for -js: %s", argv[n + 1]);
	    }
	    n += 2;
	}
	else if (arg == "-p" || arg == "-v") {
	    opts.verbose = true;
	    n++;
	}
	else if (arg[0] == '-') {
	    usage("invalid option: " + arg);
	}
	else {
	    break;
	}
    }

    // filename (required)
    if (n < argc) {
	opts.filename = argv[n];
    }
    else {
	usage("missing file name");
    }

#if MY_USE_OPENMP
    // if -jp is not specified, then use 1
    if (opts.jobs_parse < 1) {
        opts.jobs_parse = 1;
    }

    // if -js is not specified, then use 1
    if (opts.jobs_symtab < 1) {
        opts.jobs_symtab = 1;
    }
#else
    opts.jobs_parse = 1;
    opts.jobs_symtab = 1;
#endif
}

//----------------------------------------------------------------------

void
printTime(const char *label, struct timeval *tv_prev, struct timeval *tv_now,
          struct rusage *ru_prev, struct rusage *ru_now)
{
    float delta = (float)(tv_now->tv_sec - tv_prev->tv_sec)
        + ((float)(tv_now->tv_usec - tv_prev->tv_usec))/1000000.0;

    printf("%s  %8.1f sec  %8ld meg  %8ld meg", label, delta,
           (ru_now->ru_maxrss - ru_prev->ru_maxrss)/1024,
           ru_now->ru_maxrss/1024);

    cout << endl;
}

//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    struct timeval tv_init, tv_symtab, tv_parse, tv_fini;
    struct rusage  ru_init, ru_symtab, ru_parse, ru_fini;

    getOptions(argc, argv, opts);

    gettimeofday(&tv_init, NULL);
    getrusage(RUSAGE_SELF, &ru_init);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs_symtab);
#endif

    if (! Symtab::openFile(the_symtab, opts.filename)) {
        errx(1, "Symtab::openFile failed: %s", opts.filename);
    }

    the_symtab->parseTypesNow();
    the_symtab->parseFunctionRanges();

    vector <Module *> modVec;
    the_symtab->getAllModules(modVec);

#pragma omp parallel  shared(modVec)
    {
#pragma omp for  schedule(dynamic, 1)
      for (uint i = 0; i < modVec.size(); i++) {
	  modVec[i]->parseLineInformation();
      }
    }  // end parallel

    gettimeofday(&tv_symtab, NULL);
    getrusage(RUSAGE_SELF, &ru_symtab);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs_parse);
#endif

    SymtabCodeSource * code_src = new SymtabCodeSource(the_symtab);
    CodeObject * code_obj = new CodeObject(code_src);

    code_obj->parse();

    gettimeofday(&tv_parse, NULL);
    getrusage(RUSAGE_SELF, &ru_parse);

#if MY_USE_OPENMP
    omp_set_num_threads(1);
#endif

    // get function list and convert to vector.  cilk_for requires a
    // random access container.

    const CodeObject::funclist & funcList = code_obj->funcs();
    vector <ParseAPI::Function *> funcVec;

    for (auto fit = funcList.begin(); fit != funcList.end(); ++fit) {
	ParseAPI::Function * func = *fit;
	funcVec.push_back(func);
    }

    // sort funcVec to ensure deterministic output
    std::sort(funcVec.begin(), funcVec.end(), FuncLessThan);

    for (long n = 0; n < funcVec.size(); n++) {
        ParseAPI::Function * func = funcVec[n];
	doFunction(func);
    }

    cout << "\nnum funcs:  " << funcVec.size() << "\n" << endl;

    gettimeofday(&tv_fini, NULL);
    getrusage(RUSAGE_SELF, &ru_fini);

    if (opts.verbose) {
        cout << "file: " << opts.filename << "\n"
	     << "symtab threads: " << opts.jobs_symtab
	     << "  parse threads: " << opts.jobs_parse << "\n\n";

	printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
	printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
	printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
	printTime("struct:", &tv_parse, &tv_fini, &ru_parse, &ru_fini);
	printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);
	cout << endl;
    }

    return 0;
}
