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
//  This program is a proxy for the line map and inline sequence parts
//  of hpcstruct with openmp threads.
//
//  This test creates a ParseAPI::CodeObject and runs parse() in
//  parallel.  Then, it sequentially iterates through the functions,
//  blocks and instructions (no loops) and prints the raw line map and
//  inline sequences that hpcstruct would use.  Everything is sorted
//  by VMA address, so the output should be deterministic.
//
//  This program tests if ParseAPI and SymtabAPI produce deterministic
//  results for functions, blocks, stmts, line map and inline seqns,
//  but does not parse loops.  (Irreducible loops would produce
//  nondeterministic results, unless the loops are always broken in a
//  consistent, deterministic manner.)
//
//  Build me as a Dyninst application with -fopenmp, or else use
//  scripts from github.com/mwkrentel/myrepo.
//
//  Usage:
//  ./openmp-parse  [options] ...  filename
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

typedef unsigned int uint;

Symtab * the_symtab = NULL;

// Command-line options
class Options {
public:
    const char *filename;
    int   jobs;
    int   jobs_parse;
    int   jobs_symtab;
    bool  verbose;

  Options() {
	filename = NULL;
	jobs = 1;
	jobs_parse = 1;
	jobs_symtab = 1;
	verbose = false;
    }
};

Options opts;

//----------------------------------------------------------------------

// Order Blocks by start address, low to high.
static bool
BlockLessThan(Block * b1, Block * b2)
{
    return b1->start() < b2->start();
}

// Order Functions by entry address, low to high.
static bool
FuncLessThan(ParseAPI::Function * f1, ParseAPI::Function * f2)
{
    return f1->addr() < f2->addr();
}

//----------------------------------------------------------------------

void
doInstruction(Offset addr)
{
    SymtabAPI::Function * sym_func = NULL;
    Module * mod = NULL;
    vector <Statement::Ptr> svec;
    string filenm = "";
    int line = 0;

    the_symtab->getContainingFunction(addr, sym_func);
    if (sym_func != NULL) {
        mod = sym_func->getModule();
    }
    if (mod != NULL) {
        mod->getSourceLines(svec, addr);
    }

    if (! svec.empty()) {
	filenm = svec[0]->getFile();
        line = svec[0]->getLine();
    }

    cout << "stmt:  0x" << hex << addr << dec
	 << "  l=" << line
	 << "  f='" << filenm << "'\n";

    FunctionBase *func, *parent;

    if (the_symtab->getContainingInlinedFunction(addr, func) && func != NULL)
    {
	parent = func->getInlinedParent();
	while (parent != NULL) {
	    //
	    // func is inlined iff it has a parent
	    //
	    InlinedFunction * ifunc = static_cast <InlinedFunction *> (func);
	    pair <string, Offset> callsite = ifunc->getCallsite();

	    cout << "inline:  l=" << callsite.second
		 << "  f='" << callsite.first << "'"
		 << "  p='" << func->getName() << "'\n";

	    func = parent;
	    parent = func->getInlinedParent();
	}
    }
}

//----------------------------------------------------------------------

void
doBlock(Block * block)
{
    cout << "\nblock:\n";

    Dyninst::ParseAPI::Block::Insns imap;
    block->getInsns(imap);

    for (auto iit = imap.begin(); iit != imap.end(); ++iit) {
        Offset addr = iit->first;
	doInstruction(addr);
    }
}

//----------------------------------------------------------------------

void
doFunction(ParseAPI::Function * func)
{
    cout << "\n--------------------------------------------------\n"
	 << "func:  0x" << hex << func->addr() << dec
	 << "  " << func->name() << "\n";

    // vector of blocks, sorted by address
    const ParseAPI::Function::blocklist & blist = func->blocks();
    vector <Block *> blockVec;

    for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	Block * block = *bit;
	blockVec.push_back(block);
    }

    std::sort(blockVec.begin(), blockVec.end(), BlockLessThan);

    for (long n = 0; n < blockVec.size(); n++) {
        doBlock(blockVec[n]);
    }
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
    // struct jobs is always 1 for deterministic output
    opts.jobs = 1;

    // if -jp is not specified, then use 1
    if (opts.jobs_parse < 1) {
        opts.jobs_parse = 1;
    }

    // if -js is not specified, then use 1
    if (opts.jobs_symtab < 1) {
        opts.jobs_symtab = 1;
    }
#else
    opts.jobs = 1;
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
	     << "  parse threads: " << opts.jobs_parse
	     << "  struct threads: " << opts.jobs << "\n\n";

	printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
	printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
	printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
	printTime("struct:", &tv_parse, &tv_fini, &ru_parse, &ru_fini);
	printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);
	cout << endl;
    }

    return 0;
}
