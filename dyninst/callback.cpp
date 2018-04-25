//
//  Copyright (c) 2017, Rice University.
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
//  This program is a proxy for hpcstruct with full threads.
//
//  That is, it iterates through the same hierarchy of functions,
//  loops, blocks and instructions and makes the same queries that
//  hpcstruct would make, and does so via the newfunction_retstatus()
//  callback function.
//
//  This program tests two main things:
//
//  1. Does CodeObject::parse() survive threads.  This includes both
//  parse() running with cilk threads itself, and making parallel
//  queries from the newfunction callback.
//
//  2. What changes on multiple callbacks.  It is possible for parse()
//  to finalize a function and deliver its callback multiple times for
//  the same function.  We compute the set of address ranges covered
//  by the function and see if later callbacks add new blocks.
//
//  Build me as:
//  ./mk-dyninst.sh  -fcilkplus  callback.cpp  externals-dir
//
//  Usage:
//  ./callback  [options]...  filename  [ num-threads ]
//
//  Options:
//   -B           use basic blocks only, do not traverse loop tree
//   -I, -Iall    do not split basic blocks into instructions
//   -Iinline     do not compute inline callsite sequences
//   -Iline       do not compute line map info
//

#define MY_USE_CILK  1

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#if MY_USE_CILK
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#define CILK_FOR  cilk_for
#else
#define CILK_FOR  for
#endif

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
#include <ParseCallback.h>

#define MAX_VMA  0xfffffffffffffff0
#define DEFAULT_THREADS  4

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;
using namespace std;

class Range;
class FuncInfo;
class Options;

typedef unsigned long VMA;
typedef unsigned int uint;
typedef map <Block *, bool> BlockSet;
typedef map <VMA, Range> RangeSet;
typedef map <VMA, FuncInfo *> FuncMap;

Symtab * the_symtab = NULL;
FuncMap funcMap;
mutex mtx;

// Command-line options
class Options {
public:
    const char *filename;
    int   num_threads;
    bool  blocks_only;
    bool  do_instns;
    bool  do_inline;
    bool  do_linemap;

    Options() {
	filename = NULL;
	num_threads = 0;
	blocks_only = false;
	do_instns = true;
	do_inline = true;
	do_linemap = true;
    }
};

Options opts;

//----------------------------------------------------------------------

// One address range [start, end) with no attributes.
class Range {
public:
    VMA  start;
    VMA  end;

    Range(VMA st = 0, VMA en = 0) {
	start = st;
	end = en;
    }
};

// Summary info for each function.
//
// Just enough to prove that we've walked through the hierarchy of
// loops, blocks and instructions, but without creating our own inline
// tree info.
//
class FuncInfo {
public:
    string  name;
    Offset  addr;
    Offset  min_vma;
    Offset  max_vma;
    int  num_times;
    int  num_loops;
    int  num_blocks;
    int  num_instns;
    int  num_bytes;
    int  max_depth;
    int  min_line;
    int  max_line;
    RangeSet  rset;
    FuncInfo * last;

    FuncInfo(ParseAPI::Function * func) {
	name = func->name();
	addr = func->addr();
	min_vma = MAX_VMA;
	max_vma = 0;
	num_times = 0;
	num_loops = 0;
	num_blocks = 0;
	num_instns = 0;
	num_bytes = 0;
	max_depth = 0;
	min_line = 0;
	max_line = 0;
	rset.clear();
	last = NULL;
    }
};

//----------------------------------------------------------------------

void
printRangeSet(RangeSet & rset)
{
    cout << hex;

    for (auto rit = rset.begin(); rit != rset.end(); ++rit) {
	Range rng = rit->second;

	if (rit != rset.begin()) {
	    cout << "  ";
	}
	cout << "0x" << rng.start << "--0x" << rng.end;
    }
    cout << dec << "\n";
}

// Add one range to the range set (no attributes).
void
addRange(RangeSet & rset, Range range)
{
    VMA start = range.start;
    VMA end = range.end;

    if (start >= end) {
	return;
    }

    // find it = first range strictly right of start
    // left = range containing start, if there is one
    auto it = rset.upper_bound(start);
    auto left = it;
    if (left != rset.begin()) {
	--left;
    }

    if (left != rset.end() && left->second.start <= start)
    {
	if (end <= left->second.end) {
	    // new range is a subset of left
	    return;
	}

	if (start <= left->second.end) {
	    // new range overlaps with left, merge
	    start = left->second.start;
	    rset.erase(left);
	}
    }

    // delete any ranges contained within the new one
    while (it != rset.end() && it->second.end <= end) {
	auto next = it;
	++next;
	rset.erase(it);
	it = next;
    }

    if (it != rset.end() && it->second.start <= end) {
	// new range overlaps with 'it', merge
	end = std::max(end, it->second.end);
	rset.erase(it);
    }

    // add new range [start, end)
    rset[start] = Range(start, end);
}

// Compute the set of addr ranges that are contained in new_set that
// are not covered by old_set and put the answer in ans.
void
rangeSetDiff(RangeSet & old_set, RangeSet & new_set, RangeSet & ans)
{
    ans.clear();

    if (new_set.empty()) {
	return;
    }

    auto it1 = new_set.begin();
    auto it2 = old_set.begin();
    VMA start = it1->second.start;
    VMA end = it1->second.end;

    // search [start, end) plus the rest of it1+1...end for ranges not
    // covered by old_set
    for (;;) {
	if (start >= end) {
	    ++it1;
	    if (it1 == new_set.end()) {
		break;
	    }
	    start = it1->second.start;
	    end = it1->second.end;
	}

	if (it2 == old_set.end() || start < it2->second.start) {
	    // gap beginning at start
	    VMA gap_end = end;

	    if (it2 != old_set.end() && it2->second.start < end) {
		gap_end = it2->second.start;
	    }
	    addRange(ans, Range(start, gap_end));
	    start = gap_end;
	}
	else if (start < it2->second.end) {
	    // it2 covers start
	    start = it2->second.end;
	}
	else {
	    // it2 is entirely left of start
	    ++it2;
	}
    }
}

//----------------------------------------------------------------------

void
doInstruction(Offset addr, FuncInfo & finfo)
{
    finfo.num_instns++;

    // line map info (optional)
    if (opts.do_linemap) {
	SymtabAPI::Function * sym_func = NULL;
	Module * mod = NULL;
	vector <Statement::Ptr> svec;

	the_symtab->getContainingFunction(addr, sym_func);
	if (sym_func != NULL) {
	    mod = sym_func->getModule();
	}
	if (mod != NULL) {
	    mod->getSourceLines(svec, addr);
	}

	if (! svec.empty()) {
	    int line = svec[0]->getLine();

	    // line = 0 means unknown
	    if (line > 0) {
		if (line < finfo.min_line || finfo.min_line == 0) {
		    finfo.min_line = line;
		}
		finfo.max_line = std::max(finfo.max_line, line);
	    }
	}
    }

    // inline call sequence (optional)
    if (opts.do_inline) {
	FunctionBase *func, *parent;
	int depth = 0;

	if (the_symtab->getContainingInlinedFunction(addr, func) && func != NULL)
	{
	    parent = func->getInlinedParent();
	    while (parent != NULL) {
		//
		// func is inlined iff it has a parent
		//
		InlinedFunction *ifunc = static_cast <InlinedFunction *> (func);
		pair <string, Offset> callsite = ifunc->getCallsite();

		depth++;

		func = parent;
		parent = func->getInlinedParent();
	    }
	}
	finfo.max_depth = std::max(finfo.max_depth, depth);
    }
}

//----------------------------------------------------------------------

void
doBlock(Block * block, BlockSet & visited, FuncInfo & finfo)
{
    if (visited[block]) {
	return;
    }
    visited[block] = true;

    finfo.num_blocks++;
    finfo.num_bytes += block->size();
    finfo.min_vma = std::min(finfo.min_vma, block->start());
    finfo.max_vma = std::max(finfo.max_vma, block->end());

    addRange(finfo.rset, Range(block->start(), block->end()));

    // split basic block into instructions (optional)
    if (opts.do_instns) {
	map <Offset, Instruction> imap;
	block->getInsns(imap);

	for (auto iit = imap.begin(); iit != imap.end(); ++iit) {
	    Offset addr = iit->first;
	    doInstruction(addr, finfo);
	}
    }
}

//----------------------------------------------------------------------

void
doLoop(Loop * loop, BlockSet & visited, FuncInfo & finfo)
{
    finfo.num_loops++;

    // blocks in this loop
    vector <Block *> blist;
    loop->getLoopBasicBlocks(blist);

    for (uint i = 0; i < blist.size(); i++) {
	Block * block = blist[i];

	doBlock(block, visited, finfo);
    }
}

//----------------------------------------------------------------------

void
doLoopTree(LoopTreeNode * ltnode, BlockSet & visited, FuncInfo & finfo)
{
    // recur on subloops first
    vector <LoopTreeNode *> clist = ltnode->children;

    for (uint i = 0; i < clist.size(); i++) {
	doLoopTree(clist[i], visited, finfo);
    }

    // this loop last, in post order
    Loop *loop = ltnode->loop;
    doLoop(loop, visited, finfo);
}

//----------------------------------------------------------------------

void
doFunction(ParseAPI::Function * func)
{
    FuncInfo * finfo = new FuncInfo(func);

    // map of visited blocks
    const ParseAPI::Function::blocklist & blist = func->blocks();
    BlockSet visited;

    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	Block * block = *bit;
	visited[block] = false;
    }

    if (! opts.blocks_only) {
	LoopTreeNode * ltnode = func->getLoopTree();
	vector <LoopTreeNode *> clist = ltnode->children;

	// there is no top-level loop, only subtrees
	for (uint i = 0; i < clist.size(); i++) {
	    doLoopTree(clist[i], visited, *finfo);
	}
    }

    // blocks not in any loop
    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	Block * block = *bit;

	if (! visited[block]) {
	    doBlock(block, visited, *finfo);
	}
    }

    // print info for this function.
    // save first and last callbacks in funcMap.
    mtx.lock();

    cout << "\n--------------------------------------------------\n";

    auto fit = funcMap.find(finfo->addr);
    if (fit != funcMap.end()) {
	FuncInfo * first = fit->second;
	first->num_times++;

	if (first->last != NULL) {
	    delete first->last;
	}
	first->last = finfo;

	cout << "repeat func: (" << first->num_times << ")";
    }
    else {
	finfo->num_times = 1;
	funcMap[finfo->addr] = finfo;

	cout << "new func:";
    }

    cout << "  " << finfo->name << "\n"
	 << "loops:  " << finfo->num_loops
	 << "  blocks:  " << finfo->num_blocks
	 << "  instns:  " << finfo->num_instns
	 << "  bytes:  " << finfo->num_bytes << "\n"
	 << "inline depth:  " << finfo->max_depth
	 << "  line range:  " << finfo->min_line
	 << "--" << finfo->max_line << "\n"
	 << "entry:    0x" << hex << finfo->addr << "\n"
	 << "min/max:  0x" << finfo->min_vma
	 << "--0x" << finfo->max_vma << dec << "\n"
	 << "ranges:   ";

    printRangeSet(finfo->rset);

    mtx.unlock();
}

//----------------------------------------------------------------------

void
print_old_new(string label, int old_val, int new_val)
{
    cout << label << "  " << old_val;
    if (new_val != old_val) {
	cout << "  -->  " << new_val;
    }
    cout << "\n";
}

// Scan funcMap for functions that have multiple callbacks and test if
// the function has changed since its first callback.
void
printFuncDiffs()
{
    int num_funcs = 0;
    int num_repeats = 0;
    int num_diffs = 0;

    for (auto fit = funcMap.begin(); fit != funcMap.end(); ++fit) {
	FuncInfo * first = fit->second;
	FuncInfo * last = first->last;

	num_funcs++;
	if (last != NULL) { num_repeats++; }

	if (last != NULL
	    && (first->num_loops != last->num_loops
		|| first->num_blocks != last->num_blocks
		|| first->num_instns != last->num_instns
		|| first->num_bytes != last->num_bytes))
	{
	    num_diffs++;

	    cout << "\n--------------------------------------------------\n"
		 << "diff func: (" << first->num_times << ")"
		 << "  " << first->name << "\n"
		 << "entry:   0x" << hex << first->addr << dec << "\n";

	    print_old_new("loops: ", first->num_loops, last->num_loops);
	    print_old_new("blocks:", first->num_blocks, last->num_blocks);
	    print_old_new("instns:", first->num_instns, last->num_instns);
	    print_old_new("bytes: ", first->num_bytes, last->num_bytes);

	    RangeSet diff;

	    cout << "add:  ";
	    rangeSetDiff(first->rset, last->rset, diff);
	    printRangeSet(diff);

	    cout << "sub:  ";
	    rangeSetDiff(last->rset, first->rset, diff);
	    printRangeSet(diff);
	}
    }

    cout << "\ndone parsing: " << opts.filename
	 << "  num threads: " << opts.num_threads << "\n"
	 << "functions:  " << num_funcs
	 << "  repeats:  " << num_repeats
	 << "  changed:  " << num_diffs << "\n\n";
}

//----------------------------------------------------------------------

void
usage(string mesg)
{
    if (! mesg.empty()) {
	cout << "error: " << mesg << "\n\n";
    }

    cout << "usage: callback [options]... filename [num-threads]\n\n"
	 << "options:\n"
	 << "  -B           use basic blocks only, do not traverse loop tree\n"
	 << "  -I, -Iall    do not split basic blocks into instructions\n"
	 << "  -Iinline     do not compute inline callsite sequences\n"
	 << "  -Iline       do not compute line map info\n"
	 << "\n";

    exit(1);
}

// Command-line: [options]... filename [num-threads]
void
getOptions(int argc, char **argv, Options & opts)
{
    if (argc < 2) {
	usage("");
    }

    int n = 1;
    while (n < argc) {
	string arg(argv[n]);

	if (arg == "-B") {
	    opts.blocks_only = true;
	    n++;
	}
	else if (arg == "-I" || arg == "-Iall") {
	    opts.do_instns = false;
	    n++;
	}
	else if (arg == "-Iinline") {
	    opts.do_inline = false;
	    n++;
	}
	else if (arg == "-Iline") {
	    opts.do_linemap = false;
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
    n++;

    // num threads (optional)
    char *str = getenv("CILK_NWORKERS");
    if (n < argc) {
	opts.num_threads = atoi(argv[n]);
    }
    else if (str != NULL) {
	opts.num_threads = atoi(str);
    }
    else {
	opts.num_threads = DEFAULT_THREADS;
    }
    if (opts.num_threads <= 0) {
	usage("bad value for num threads");
    }
}

//----------------------------------------------------------------------

void
printTime(const char *label, struct timeval *tv_prev, struct timeval *tv_now,
	  struct rusage *ru_prev, struct rusage *ru_now)
{
    float delta = (float)(tv_now->tv_sec - tv_prev->tv_sec)
	+ ((float)(tv_now->tv_usec - tv_prev->tv_usec))/1000000.0;

    printf("%s  %8.1f sec  %8ld meg  %8ld meg\n", label, delta,
	   (ru_now->ru_maxrss - ru_prev->ru_maxrss)/1024,
	   ru_now->ru_maxrss/1024);
}

//----------------------------------------------------------------------

class MyCallback : public ParseCallback {
public:
    virtual void
    newfunction_retstatus(ParseAPI::Function * func)
    {
	doFunction(func);
    }
};

int
main(int argc, char **argv)
{
    struct timeval tv_init, tv_symtab, tv_parse;
    struct rusage  ru_init, ru_symtab, ru_parse;

    getOptions(argc, argv, opts);

#if MY_USE_CILK
    // cilk set_param requires char * for value, not int
    char buf[10];
    snprintf(buf, 10, "%d", opts.num_threads);

    int ret = __cilkrts_set_param("nworkers", buf);
    if (ret != __CILKRTS_SET_PARAM_SUCCESS) {
	errx(1, "__cilkrts_set_param failed: %d", ret);
    }
#else
    opts.num_threads = 1;
#endif

    gettimeofday(&tv_init, NULL);
    getrusage(RUSAGE_SELF, &ru_init);

    cout << "begin parsing: " << opts.filename << "\n"
	 << "num threads: " << opts.num_threads << "\n";

    if (! Symtab::openFile(the_symtab, opts.filename)) {
	errx(1, "Symtab::openFile failed: %s", opts.filename);
    }
    the_symtab->parseTypesNow();
    the_symtab->parseFunctionRanges();

    vector <Module *> modVec;
    the_symtab->getAllModules(modVec);

    for (auto mit = modVec.begin(); mit != modVec.end(); ++mit) {
	(*mit)->parseLineInformation();
    }

    gettimeofday(&tv_symtab, NULL);
    getrusage(RUSAGE_SELF, &ru_symtab);

    SymtabCodeSource * code_src = new SymtabCodeSource(the_symtab);
    MyCallback * mycall = new MyCallback();
    CodeObject * code_obj = new CodeObject(code_src, NULL, mycall, false);

    // delivers callbacks to doFunction()
    code_obj->parse();

    gettimeofday(&tv_parse, NULL);
    getrusage(RUSAGE_SELF, &ru_parse);

    printFuncDiffs();

    printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
    printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
    printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
    printTime("total: ", &tv_init, &tv_parse, &ru_init, &ru_parse);

    return 0;
}
