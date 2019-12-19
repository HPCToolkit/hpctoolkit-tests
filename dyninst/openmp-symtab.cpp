//
//  Copyright (c) 2019, Rice University.
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
//  This program is a proxy for the parallel symtab + elfutils phase
//  of hpcstruct with openmp threads.  It tests that symtab doesn't
//  crash and produces deterministic output for symbol names and
//  addresses, line map info and inline sequences.
//
//  By default, all output is printed (symbol names, addresses, line
//  map info and inline sequences), but there are options for turning
//  off various parts.
//
//  Mark W. Krentel
//  December 2019
//
//  Usage:
//  ./openmp-symtab  [options] ...  filename
//
//  Options:
//   -h         display usage message and exit
//   -j  num    use num openmp threads
//   -jl num    use num threads for line map
//   -M         read file into memory before passing to symtab
//   -A         print only symbol name and address
//   -I         disable printing inline sequences
//   -L         disable printing line map info
//   -N         disable printing all symbol names
//   -S         disable printing any statement info
//   -T         print time and space usage to stderr
//   -X num     print statements every num bytes
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
#include <vector>

#include <Symtab.h>
#include <Function.h>
#include <Module.h>
#include <LineInformation.h>

typedef unsigned long VMA;
typedef unsigned int uint;

#define MAX_VMA  0xfffffffffffffff0

using namespace Dyninst;
using namespace SymtabAPI;
using namespace std;

Symtab * the_symtab = NULL;
char * mem_image = NULL;

// Command-line options
class Options {
public:
    const char *filename;
    int   jobs;
    int   jobs_linemap;
    int   instn_len;
    bool  disable_all;
    bool  do_memory;
    bool  do_full_names;
    bool  do_statements;
    bool  do_linemap;
    bool  do_inline;
    bool  do_print_time;

    Options() {
	filename = NULL;
	jobs = -1;
	jobs_linemap = -1;
	instn_len = 4;
	disable_all = false;
	do_memory = false;
	do_full_names = true;
	do_statements = true;
	do_linemap = true;
	do_inline = true;
	do_print_time = false;
    }
};

Options opts;

//----------------------------------------------------------------------

// Sort Symtab Functions by address, low to high.
static bool
FuncLessThan(Function * f1, Function * f2)
{
    return f1->getOffset() < f2->getOffset();
}

//----------------------------------------------------------------------

void
doInstruction(Function * sym_func, VMA addr)
{
    Module * mod = sym_func->getModule();
    vector <Statement::Ptr> svec;

    string filenm = "";
    int line = 0;

    if (mod != NULL) {
	mod->getSourceLines(svec, addr);
    }

    if (! svec.empty()) {
        filenm = svec[0]->getFile();
        line = svec[0]->getLine();
    }

    cout << "stmt:  0x" << hex << addr << dec;

    if (opts.do_linemap) {
	cout << "  l=" << line << "  f='" << filenm << "'";
    }
    cout << "\n";

    if (opts.do_inline) {
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

		cout << "   inline:  l=" << callsite.second
		     << "  f='" << callsite.first << "'"
		     << "  p='" << func->getName() << "'\n";

		func = parent;
		parent = func->getInlinedParent();
	    }
	}
    }
}

//----------------------------------------------------------------------

// If a Function has multiple names, choose one deterministically.
// Select the longest mangled name, break ties by alphabetically
// lowest.  This is what hpcstruct does.
//
string
singleName(Function * func)
{
    string name ("unknown");
    auto mangled_begin = func->mangled_names_begin();

    for (auto mit = mangled_begin; mit != func->mangled_names_end(); ++mit)
    {
	string new_name = *mit;

	if (mit == mangled_begin
	    || (new_name.length() > name.length())
	    || (new_name.length() == name.length()
		&& new_name.compare(name) < 0))
	{
	    name = new_name;
	}
    }

    return name;
}

//----------------------------------------------------------------------

// Sort the names for deterministic output.
void
printNames(Aggregate::name_iter begin_it, Aggregate::name_iter end_it)
{
    vector <string> nameVec;

    for (auto it = begin_it; it != end_it; ++it) {
	nameVec.push_back(*it);
    }

    std::sort(nameVec.begin(), nameVec.end());

    for (auto it = nameVec.begin(); it != nameVec.end(); ++it) {
	cout << *it << "\n";
    }
}

//----------------------------------------------------------------------

void
doFunction(Function * func, Function * next_func)
{
    VMA start_vma = func->getOffset();
    VMA end_vma = start_vma + func->getSize();

    // end addr can't extend past start addr of next function
    if (next_func != NULL) {
	VMA next_start = next_func->getOffset();

	if (next_start > start_vma) {
	    end_vma = std::min(end_vma, next_start);
	}
    }

    // end addr can't extend past end of region
    Region * region = func->getRegion();
    if (region != NULL) {
	VMA reg_end = region->getMemOffset() + region->getMemSize();

	if (reg_end > start_vma) {
	    end_vma = std::min(end_vma, reg_end);
	}
    }

    if (opts.disable_all) {
	cout << "0x" << hex << start_vma << "--0x" << end_vma << dec
	     << "  " << singleName(func) << "\n";
	return;
    }

    cout << "\n--------------------------------------------------\n"
	 << "0x" << hex << start_vma << "--0x" << end_vma << dec
	 << "  " << singleName(func) << "\n";

    if (next_func != NULL && start_vma == next_func->getOffset()) {
	cout << "\nwarning: next function starts at same address\n"
	     << "0x" << hex << next_func->getOffset() << dec
	     << "  " << singleName(next_func) << "\n";
    }

    if (opts.do_full_names) {
	cout << "\nmangled names:\n";
	printNames(func->mangled_names_begin(), func->mangled_names_end());

	cout << "\npretty names:\n";
	printNames(func->pretty_names_begin(), func->pretty_names_end());

	cout << "\ntyped names:\n";
	printNames(func->typed_names_begin(), func->typed_names_end());
    }

    if (opts.do_statements) {
	cout << "\n";
	for (VMA vma = start_vma; vma < end_vma; vma += opts.instn_len) {
	    doInstruction(func, vma);
	}
    }
}

//----------------------------------------------------------------------

void
openSymtab(void)
{
    if (opts.do_memory) {
	//
	// read filename into memory and pass to Symtab as a memory
	// buffer.  this is what hpcstruct does.  (-M option)
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
	// let Symtab read the file itself (default).
	//
	if (! Symtab::openFile(the_symtab, opts.filename)) {
	    errx(1, "Symtab::openFile (on disk) failed: %s", opts.filename);
	}
    }
}

//----------------------------------------------------------------------

void
usage(string mesg)
{
    if (! mesg.empty()) {
	cout << "error: " << mesg << "\n\n";
    }

    cout << "usage:  openmp-symtab  [options]...  filename\n\n"
	 << "options:\n"
	 << "  -h         display usage message and exit\n"
	 << "  -j  num    use num openmp threads\n"
	 << "  -jl num    use num threads for line map\n"
	 << "  -M         read file into memory before passing to symtab\n"
	 << "  -A         print only symbol name and address\n"
	 << "  -I         disable printing inline sequences\n"
	 << "  -L         disable printing line map info\n"
	 << "  -N         disable printing all symbol names\n"
	 << "  -S         disable printing any statement info\n"
	 << "  -T         print time and space usage to stderr\n"
	 << "  -X num     print statements every num bytes\n"
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
	else if (arg == "-jl") {
	    if (n + 1 >= argc) {
	        usage("missing arg for -jl");
	    }
	    opts.jobs_linemap = atoi(argv[n + 1]);
	    if (opts.jobs_linemap <= 0) {
	        errx(1, "bad arg for -jl: %s", argv[n + 1]);
	    }
	    n += 2;
	}
	else if (arg == "-X") {
	    if (n + 1 >= argc) {
	        usage("missing arg for -X");
	    }
	    opts.instn_len = atoi(argv[n + 1]);
	    if (opts.instn_len <= 0) {
	        errx(1, "bad arg for -X: %s", argv[n + 1]);
	    }
	    n += 2;
	}
	else if (arg == "-A") {
	    opts.disable_all = true;
	    n++;
	}
	else if (arg == "-M") {
	    opts.do_memory = true;
	    n++;
	}
	else if (arg == "-N") {
	    opts.do_full_names = false;
	    n++;
	}
	else if (arg == "-S") {
	    opts.do_statements = false;
	    n++;
	}
	else if (arg == "-L") {
	    opts.do_linemap = false;
	    n++;
	}
	else if (arg == "-I") {
	    opts.do_inline = false;
	    n++;
	}
	else if (arg == "-T") {
	    opts.do_print_time = true;
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
        opts.jobs = std::min(omp_get_max_threads(), (int) 16);
    }

    // if -jl is not specified, then use -j
    if (opts.jobs_linemap < 1) {
        opts.jobs_linemap = opts.jobs;
    }

#else
    opts.jobs = 1;
    opts.jobs_linemap = 1;
#endif
}

//----------------------------------------------------------------------

void
printTime(const char *label, struct timeval *tv_prev, struct timeval *tv_now,
	  struct rusage *ru_prev, struct rusage *ru_now)
{
    float delta = (float)(tv_now->tv_sec - tv_prev->tv_sec)
	+ ((float)(tv_now->tv_usec - tv_prev->tv_usec))/1000000.0;

    fprintf(stderr, "%s  %8.1f sec  %8ld meg  %8ld meg", label, delta,
	    (ru_now->ru_maxrss - ru_prev->ru_maxrss)/1024,
	    ru_now->ru_maxrss/1024);

    cerr << endl;
}

//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    struct timeval tv_init, tv_symtab, tv_linemap;
    struct rusage  ru_init, ru_symtab, ru_linemap;

    getOptions(argc, argv, opts);

    gettimeofday(&tv_init, NULL);
    getrusage(RUSAGE_SELF, &ru_init);

    //
    // Open and Analyze Symtab
    //
#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs);
#endif

    openSymtab();

    the_symtab->parseTypesNow();
    the_symtab->parseFunctionRanges();

    gettimeofday(&tv_symtab, NULL);
    getrusage(RUSAGE_SELF, &ru_symtab);

    //
    // Compute Line Map Info
    //
    vector <Module *> modVec;
    the_symtab->getAllModules(modVec);

#if MY_USE_OPENMP
    omp_set_num_threads(opts.jobs_linemap);
#endif

#pragma omp parallel  shared(modVec)
    {
#pragma omp for  schedule(dynamic, 1)
      for (uint i = 0; i < modVec.size(); i++) {
	  modVec[i]->parseLineInformation();
      }
    }  // end parallel

    gettimeofday(&tv_linemap, NULL);
    getrusage(RUSAGE_SELF, &ru_linemap);

    //
    // Print Symtab and Line Map Info
    //
    vector <Function *> FuncVec;

    the_symtab->getAllFunctions(FuncVec);

    std::sort(FuncVec.begin(), FuncVec.end(), FuncLessThan);

    for (auto fit = FuncVec.begin(); fit != FuncVec.end(); ++fit) {
	auto next_fit = fit;  next_fit++;
	Function * func = *fit;
	Function * next_func = (next_fit != FuncVec.end()) ? *next_fit : NULL;

	doFunction(func, next_func);
    }

    cout << "\nnum functions:  " << FuncVec.size() << "\n" << endl;

    if (opts.do_print_time) {
	cerr << "file:  " << opts.filename << "\n"
	     << "symtab threads:  " << opts.jobs
	     << "  linemap threads:  " << opts.jobs_linemap << "\n\n";

	printTime("init:   ", &tv_init, &tv_init, &ru_init, &ru_init);
	printTime("symtab: ", &tv_init, &tv_symtab, &ru_init, &ru_symtab);
	printTime("linemap:", &tv_symtab, &tv_linemap, &ru_symtab, &ru_linemap);
	printTime("total:  ", &tv_init, &tv_linemap, &ru_init, &ru_linemap);
	cerr << endl;
    }

    return 0;
}
