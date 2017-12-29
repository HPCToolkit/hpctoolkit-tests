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
//  This program is a proxy for hpcstruct with openmp threads.
//
//  Iterate through the same hierarchy of functions, loops, blocks,
//  instructions, inline call sequences and line map info, and collect
//  (mostly) the same raw data that hpcstruct would collect.  But
//  don't build a full inline tree, instead just collect summary info
//  for each function.
//
//  This program tests that ParseAPI and SymtabAPI can be run in
//  parallel with openmp threads.  For now, we parse the entire binary
//  sequentially (unless ParseAPI is built with threads) and then make
//  parallel queries.
//
//  Build me as:
//  ./mk-dyninst.sh  -fopenmp  openmp-parse.cpp  externals-dir
//
//  Usage:
//  ./openmp-parse  [options]...  filename  [ num-threads ]
//
//  Options:
//   -p           print function information
//   -I, -Iall    do not split basic blocks into instructions
//   -Iinline     do not compute inline callsite sequences
//   -Iline       do not compute line map info
//


#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <omp.h>

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
#define DEFAULT_THREADS  4

#define NT "OMP_NUM_THREADS"

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;
using namespace std;

typedef map <Block *, bool> BlockSet;
typedef unsigned int uint;

Symtab * the_symtab = NULL;
mutex mtx;

// Command-line options
class Options {
public:
    const char *filename;
    bool  print;
    bool  do_instns;
    bool  do_inline;
    bool  do_linemap;

    Options() {
	print = false;
	filename = NULL;
	do_instns = true;
	do_inline = true;
	do_linemap = true;
    }
};

Options opts;

//----------------------------------------------------------------------

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
    int  num_loops;
    int  num_blocks;
    int  num_instns;
    int  max_depth;
    int  min_line;
    int  max_line;

    FuncInfo(ParseAPI::Function * func) {
	name = func->name();
	addr = func->addr();
	min_vma = MAX_VMA;
	max_vma = 0;
	num_loops = 0;
	num_blocks = 0;
	num_instns = 0;
	max_depth = 0;
	min_line = 0;
	max_line = 0;
    }
};

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
    finfo.min_vma = std::min(finfo.min_vma, block->start());
    finfo.max_vma = std::max(finfo.max_vma, block->end());

    // split basic block into instructions (optional)
    if (opts.do_instns) {
	map <Offset, InstructionAPI::InstructionPtr> imap;
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
    FuncInfo finfo(func);

    // map of visited blocks
    const ParseAPI::Function::blocklist & blist = func->blocks();
    BlockSet visited;

    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	Block * block = *bit;
	visited[block] = false;
    }

    LoopTreeNode * ltnode = func->getLoopTree();
    vector <LoopTreeNode *> clist = ltnode->children;

    // there is no top-level loop, only subtrees
    for (uint i = 0; i < clist.size(); i++) {
	doLoopTree(clist[i], visited, finfo);
    }

    // blocks not in any loop
    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	Block * block = *bit;

	if (! visited[block]) {
	    doBlock(block, visited, finfo);
	}
    }

    if (opts.print) {
      // print info for this function
      mtx.lock();

      cout << "\n--------------------------------------------------\n"
	   << hex
	   << "func:  0x" << finfo.addr << "  " << finfo.name << "\n"
	   << "0x" << finfo.min_vma << "--0x" << finfo.max_vma << "\n"
	   << dec
	   << "loops:  " << finfo.num_loops
	   << "  blocks:  " << finfo.num_blocks
	   << "  instns:  " << finfo.num_instns << "\n"
	   << "inline depth:  " << finfo.max_depth
	   << "  line range:  " << finfo.min_line << "--" << finfo.max_line
	   << "\n";

      mtx.unlock();
    }
}

//----------------------------------------------------------------------

void
usage(string mesg)
{
    if (! mesg.empty()) {
	cout << "error: " << mesg << "\n\n";
    }

    cout << "usage: cilk-parse [options]... filename [num-threads]\n\n"
	 << "options:\n"
         << "  -p           print function information\n"
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

	if (arg == "-p") {
	    opts.print = true;
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

int
main(int argc, char **argv)
{
    struct timeval tv_init, tv_symtab, tv_parse, tv_fini;
    struct rusage  ru_init, ru_symtab, ru_parse, ru_fini;

    getOptions(argc, argv, opts);

    gettimeofday(&tv_init, NULL);
    getrusage(RUSAGE_SELF, &ru_init);

    cout << "begin open: " << opts.filename << "\n"
         << "OpenMP threads: " << omp_get_max_threads() << std::endl;

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
    CodeObject * code_obj = new CodeObject(code_src);

    printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
    code_obj->parse();

    gettimeofday(&tv_parse, NULL);
    getrusage(RUSAGE_SELF, &ru_parse);

    printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);

    // get function list and convert to vector.  cilk_for requires a
    // random access container.

    const CodeObject::funclist & funcList = code_obj->funcs();
    vector <ParseAPI::Function *> funcVec;

    for (auto fit = funcList.begin(); fit != funcList.end(); ++fit) {
	ParseAPI::Function * func = *fit;
	funcVec.push_back(func);
    }

#pragma omp parallel for
    for (long n = 0; n < funcVec.size(); n++) {
	ParseAPI::Function * func = funcVec[n];
	doFunction(func);
    }

    gettimeofday(&tv_fini, NULL);
    getrusage(RUSAGE_SELF, &ru_fini);

    cout << "\ndone parsing: " << opts.filename << "\n"
	 << "num threads: " << omp_get_max_threads()
	 << "  num funcs: " << funcVec.size() << "\n\n";

    printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
    printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
    printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
    printTime("struct:", &tv_parse, &tv_fini, &ru_parse, &ru_fini);
    printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);

    return 0;
}
