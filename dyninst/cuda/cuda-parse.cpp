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
//  Now modified to be a proxy for CodeObject and parse for CUDA.
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
//   -p, -v       print verbose function information
//   -D           disable delete CodeObject and Symtab CodeSource
//   -M           disable read() file in memory before openFile()
//   -I, -Iall    do not split basic blocks into instructions
//   -Iinline     do not compute inline callsite sequences
//   -Iline       do not compute line map info
//   -h, --help   display usage message and exit
//

#define MY_USE_OPENMP  0

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

#include "ElfHelper.hpp"
#include "Fatbin.hpp"
#include "InputFile.hpp"
#include "RelocateCubin.hpp"
#include "CudaCFGFactory.hpp"
#include "CudaFunction.hpp"
#include "CudaBlock.hpp"
#include "CudaCodeSource.hpp"
#include "GraphReader.hpp"
#include "CFGParser.hpp"

#define MAX_VMA  0xfffffffffffffff0

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;
using namespace std;

typedef map <Block *, bool> BlockSet;
typedef unsigned int uint;

// Command-line options
class Options {
public:
    const char *filename;
    int   jobs;
    int   jobs_parse;
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
	verbose = false;
	do_delete = true;
	do_memory = true;
	do_instns = true;
	do_inline = true;
	do_linemap = true;
    }
};

static Symtab * the_symtab = NULL;
static Options opts;
static mutex mtx;

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

    FuncInfo(ParseAPI::Function * func = NULL) {
	name = (func != NULL) ? func->name() : "";
	addr = (func != NULL) ? func->addr() : 0;
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
      std::cout << line << std::endl;

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
          std::cout << "Line mapping: " << addr << "->";
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
doFunction(ParseAPI::Function * func, FuncInfo & summary)
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

    mtx.lock();

    // info for this function
    if (opts.verbose) {
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
    }

    // summary of all functions
    summary.min_vma = std::min(summary.min_vma, finfo.min_vma);
    summary.max_vma = std::max(summary.max_vma, finfo.max_vma);
    summary.num_loops  += finfo.num_loops;
    summary.num_blocks += finfo.num_blocks;
    summary.num_instns += finfo.num_instns;
    summary.max_depth = std::max(summary.max_depth, finfo.max_depth);
    summary.min_line =  std::min(summary.min_line,  finfo.min_line);
    summary.max_line =  std::max(summary.max_line,  finfo.max_line);

    mtx.unlock();
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
#else
    opts.jobs = 1;
    opts.jobs_parse = 1;
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
    string filename = opts.filename;

    cout << "begin open: " << filename << "\n"
	 << "parse threads: " << opts.jobs_parse
	 << "  struct threads: " << opts.jobs << "\n";

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs);
#endif

    InputFile inputFile;
    if (! inputFile.openFile(filename)) {
        errx(1, "InputFile::inputFile() failed: %s", opts.filename);
    }

    ElfFileVector * elfFileVector = inputFile.fileVector();
    if (elfFileVector == NULL || elfFileVector->empty()) {
        errx(1, "elfFileVector is empty");
    }

    for (uint i = 0; i < elfFileVector->size(); i++) {
        ElfFile * elfFile = (*elfFileVector)[i];
	string elf_name = elfFile->getFileName();
	char * elf_addr = elfFile->getMemory();
	size_t elf_len = elfFile->getLength();

	cout << "\n------------------------------------------------------------\n"
	     << "Elf File:  " << elf_name << "\n"
	     << "length:    0x" << hex << elf_len << dec << "  (" << elf_len << ")\n"
	     << "------------------------------------------------------------"
	     << endl;

	gettimeofday(&tv_init, NULL);
	getrusage(RUSAGE_SELF, &ru_init);

#if MY_USE_OPENMP
	omp_set_num_threads(opts.jobs_parse);
#endif

	Symtab::openFile(the_symtab, elf_addr, elf_len, elf_name);
	if (the_symtab == NULL) {
	    cout << "warning: Symtab::openFile() failed\n";
	    continue;
	}
	bool cuda_file = (the_symtab->getArchitecture() == Dyninst::Arch_cuda);

	the_symtab->parseTypesNow();
	the_symtab->parseFunctionRanges();

	vector <Module *> modVec;
	the_symtab->getAllModules(modVec);

	for (auto mit = modVec.begin(); mit != modVec.end(); ++mit) {
	    (*mit)->parseLineInformation();
	}

	gettimeofday(&tv_symtab, NULL);
	getrusage(RUSAGE_SELF, &ru_symtab);

	CodeSource * code_src = NULL;
	CodeObject * code_obj = NULL;
  CFGFactory * cfg_fact = NULL;

	if (cuda_file) {
      std::string relocated_cubin = filename + ".relocated";
      int fd = open(relocated_cubin.c_str(), O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
      if (write(fd, elf_addr, elf_len) != elf_len) {
          cout << "Write " << relocated_cubin << " to disk failed" << endl; 
          continue;
      }
      close(fd);
      std::string relocated_dot = filename + ".dot";
      std::string cmd = "nvdisasm -cfg -poff " + filename + " > " + relocated_dot;
      FILE *output = popen(cmd.c_str(), "r");
      if (!output) {
          cout << "Dump " << relocated_dot << " to disk failed" << endl; 
          continue;
      }
      pclose(output);

      // parse dot cfg
      CudaParse::GraphReader graph_reader(relocated_dot);
      CudaParse::Graph graph;
      graph_reader.read(graph);
      CudaParse::CFGParser cfg_parser;
      std::vector<CudaParse::Function *> functions;
      cfg_parser.parse(graph, functions);

      // relocate instructions
      std::vector<Symbol *> symbols;
      the_symtab->getAllSymbols(symbols);
      for (auto *symbol : symbols) {
        for (auto *function : functions) {
          if (function->name == symbol->getMangledName()) {
            for (auto *block : function->blocks) {
              for (auto *inst : block->insts) {
                inst->offset += symbol->getOffset();
              }
            }
          }
        }
      }

      cfg_fact = new CudaCFGFactory(functions);
      code_src = new CudaCodeSource(functions); 
      code_obj = new CodeObject(code_src, cfg_fact);
      code_obj->parse();
      for (auto *function : functions) {
        delete function;
      }
	}
	else {
	    code_src = new SymtabCodeSource(the_symtab);
	    code_obj = new CodeObject(code_src);
	    code_obj->parse();
	}

	gettimeofday(&tv_parse, NULL);
	getrusage(RUSAGE_SELF, &ru_parse);

#if MY_USE_OPENMP
	omp_set_num_threads(opts.jobs);
#endif

	const CodeObject::funclist & funcList = code_obj->funcs();
	vector <ParseAPI::Function *> funcVec;
	FuncInfo summary;

	for (auto fit = funcList.begin(); fit != funcList.end(); ++fit) {
	    ParseAPI::Function * func = *fit;
	    funcVec.push_back(func);
	}

#pragma omp parallel for
	for (long n = 0; n < funcVec.size(); n++) {
	    ParseAPI::Function * func = funcVec[n];
	    doFunction(func, summary);
	}

	gettimeofday(&tv_fini, NULL);
	getrusage(RUSAGE_SELF, &ru_fini);
	    
	cout << "\nsummary:  funcs:  " << funcList.size() << "\n"
	     << "loops:  " << summary.num_loops
	     << "  blocks:  " << summary.num_blocks
	     << "  instns:  " << summary.num_instns << "\n"
	     << "inline depth:  " << summary.max_depth
	     << "  line range:  " << summary.min_line << "--" << summary.max_line
	     << "\n\n";

	printTime("init:  ", &tv_init, &tv_init, &ru_init, &ru_init);
	printTime("symtab:", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
	printTime("parse: ", &tv_symtab, &tv_parse, &ru_symtab, &ru_parse);
	printTime("total: ", &tv_init, &tv_fini, &ru_init, &ru_fini);

  if (cfg_fact != NULL) {
      delete cfg_fact;
  }
	if (code_obj != NULL) {
	    delete code_obj;
	}
	if (code_src != NULL) {
      if (cuda_file == true) {
          delete (CudaCodeSource *)code_src;
      } else {
          delete (SymtabCodeSource *)code_src;
      }
	}

	Symtab::closeSymtab(the_symtab);
    }

    cout << endl;

    return 0;
}
