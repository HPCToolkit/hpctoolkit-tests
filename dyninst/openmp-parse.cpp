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
//  This program is a proxy for hpcstruct with openmp threads.
//
//  Iterate through the same hierarchy of functions, loops, blocks,
//  instructions, inline call sequences and line map info, and collect
//  (mostly) the same raw data that hpcstruct would collect.  But
//  don't build a full inline tree, instead just collect summary info
//  for each function.
//
//  This program tests that ParseAPI and SymtabAPI can be run in
//  parallel with openmp threads.  There are now two parallel phases:
//  parse() a CodeObject inside ParseAPI, and the doFunction() loop as
//  a proxy for hpcstruct queries.
//
//  Use scripts from github.com/mwkrentel/myrepo to build me.
//
//  Usage:
//  ./openmp-parse  [options] ...  filename
//
//  Options:
//   -j  num      use <num> openmp threads
//   -jp num      use <num> threads for ParseAPI::parse()
//   -js num      use <num> threads for Symtab methods
//   -p, -v       print verbose function information
//   -D           disable delete CodeObject and Symtab CodeSource
//   -M           disable read() file in memory before openFile()
//   -I, -Iall    do not split basic blocks into instructions
//   -Iinline     do not compute inline callsite sequences
//   -Iline       do not compute line map info
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

typedef map <Block *, bool> BlockSet;
typedef unsigned int uint;

Symtab * the_symtab = NULL;
mutex mtx;

// Command-line options
class Options {
public:
    const char *filename;
    int   jobs;
    int   jobs_parse;
    int   jobs_symtab;
    bool  verbose;
    bool  do_delete;
    bool  do_memory;
    bool  do_instns;
    bool  do_inline;
    bool  do_linemap;

    Options() {
	filename = NULL;
	jobs = -1;
	jobs_parse = -1;
	jobs_symtab = -1;
	verbose = false;
	do_delete = true;
	do_memory = true;
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
 	Dyninst::ParseAPI::Block::Insns imap;
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

    if (opts.verbose) {
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

    cout << "usage:  openmp-parse  [options]...  filename\n\n"
	 << "options:\n"
	 << "  -j  num      use num openmp threads\n"
	 << "  -jp num      use num threads for ParseAPI::parse()\n"
	 << "  -p, -v       print verbose function information\n"
	 << "  -D           disable delete CodeObject and Sybtab CodeSource\n"
	 << "  -M           dsable read() file in memory before openFile\n"
	 << "  -I, -Iall    do not split basic blocks into instructions\n"
	 << "  -Iinline     do not compute inline callsite sequences\n"
	 << "  -Iline       do not compute line map info\n"
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
	else if (arg == "-j") {
	    if (n + 1 >= argc) {
	        usage("missing arg for -j");
	    }
	    opts.jobs = atoi(argv[n + 1]);
	    if (opts.jobs <= 0) {
	        errx(1, "bad arg for -j: %s", argv[n + 1]);
	    }
	    n += 2;
	}
	else if (arg == "-jp") {
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
	else if (arg == "-D") {
	    opts.do_delete = false;
	    n++;
	}
	else if (arg == "-M") {
	    opts.do_memory = false;
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

#if MY_USE_OPENMP
    // if -j is not specified, then ask the runtime library.
    if (opts.jobs < 1) {
        opts.jobs = omp_get_max_threads();
    }

    // if -jp is not specified, then use -j for both.
    if (opts.jobs_parse < 1) {
        opts.jobs_parse = opts.jobs;
    }

    // if -js is not specified, then use -jp
    if (opts.jobs_symtab < 1) {
	opts.jobs_symtab = opts.jobs_parse;
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

    cout << "begin open: " << opts.filename << "\n"
	 << "symtab threads: " << opts.jobs_symtab
	 << "  parse threads: " << opts.jobs_parse
	 << "  struct threads: " << opts.jobs << "\n" << endl;

    gettimeofday(&tv_init, NULL);
    getrusage(RUSAGE_SELF, &ru_init);
    printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs_symtab);
#endif

    char * mem_image = NULL;

    if (opts.do_memory) {
	//
	// read filename into memory and pass to Symtab as a memory
	// buffer.  this is what hpcstruct does.
	//
	int fd = open(opts.filename, O_RDONLY);
	if (fd < 0) {
	    err(1, "unable to open: %s", opts.filename);
	}

	struct stat sb;
	int ret = fstat(fd, &sb);
	if (ret != 0) {
	    err(1, "unable to fstat: %s", opts.filename);
	}
	size_t file_size = sb.st_size;

	mem_image = (char *) malloc(file_size);
	if (mem_image == NULL) {
	    err(1, "unable to malloc %ld bytes", file_size);
	}

	ssize_t len = read(fd, mem_image, file_size);
	if (len < file_size) {
	    err(1, "read only %ld out of %ld bytes", len, file_size);
	}
	close(fd);

	if (! Symtab::openFile(the_symtab, mem_image, file_size, opts.filename)) {
	    errx(1, "Symtab::openFile (in memory) failed: %s", opts.filename);
	}
    }
    else {
	//
	// let Symtab read the file itself (-M option).
	//
	if (! Symtab::openFile(the_symtab, opts.filename)) {
	    errx(1, "Symtab::openFile (on disk) failed: %s", opts.filename);
	}
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
    printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs_parse);
#endif

    SymtabCodeSource * code_src = new SymtabCodeSource(the_symtab);
    CodeObject * code_obj = new CodeObject(code_src);

    code_obj->parse();

    gettimeofday(&tv_parse, NULL);
    getrusage(RUSAGE_SELF, &ru_parse);
    printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs);
#endif

    // get function list and convert to vector.  cilk_for requires a
    // random access container.

    const CodeObject::funclist & funcList = code_obj->funcs();
    vector <ParseAPI::Function *> funcVec;

    for (auto fit = funcList.begin(); fit != funcList.end(); ++fit) {
	ParseAPI::Function * func = *fit;
	funcVec.push_back(func);
    }

#pragma omp parallel  shared(funcVec)
    {
#pragma omp for  schedule(dynamic, 1)
      for (long n = 0; n < funcVec.size(); n++) {
	  ParseAPI::Function * func = funcVec[n];
	  doFunction(func);
      }
    }  // end parallel

    if (opts.do_delete) {
	if (opts.verbose) {
	    cout << "\ndelete CodeObject, CodeSource, close Symtab ..." << endl;
	}
	delete code_obj;
	delete code_src;
	Symtab::closeSymtab(the_symtab);
	if (opts.do_memory) {
	    free(mem_image);
	}
    }

    gettimeofday(&tv_fini, NULL);
    getrusage(RUSAGE_SELF, &ru_fini);
    if (! opts.verbose) {
	printTime("struct:", &tv_parse, &tv_fini, &ru_parse, &ru_fini);
	printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);
    }

    cout << "\ndone parsing: " << opts.filename << "\n"
	 << "num threads: " << opts.jobs_symtab
	 << ", " << opts.jobs_parse << ", " << opts.jobs
	 << "  num funcs: " << funcVec.size() << "\n" << endl;

    if (opts.verbose) {
	printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
	printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
	printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
	printTime("struct:", &tv_parse, &tv_fini, &ru_parse, &ru_fini);
	printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);
	cout << endl;
    }

    return 0;
}
