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
//  This program tries to find gaps in the ParseAPI parsing, that is,
//  address ranges that are not covered by the basic blocks of any
//  ParseAPI Function that may contain code.
//
//  We construct two sets of addresses ranges: (1) ranges from the
//  SymtabAPI Function symbols, and (2) ranges covered by the basic
//  blocks of the ParseAPI Functions.
//
//  Then, we identify address ranges within a SymtabAPI Function that
//  are not covered by any basic block.  We apply the following
//  heuristics to help decide if these gaps may be code regions.
//
//  1. size of gap -- larger is more suspicious.
//  2. location of gap -- end of function is less suspicious.
//  3. is gap in PLT region -- if yes, this is less suspicious.
//  4. out edges from block before gap -- sink is suspicious.
//  5. line map info -- if exists, this is very suspicious.
//
//  Build me as:
//  ./mk-dyninst.sh  find-gaps.cpp  externals-dir
//
//  Usage:
//  ./find-gaps  [option...]  filename
//
//  Options:
//  -D  print raw input from symtab and parseapi for regions, modules,
//      symbols and functions for debugging.
//
// ----------------------------------------------------------------------
//
//  Todo:
//  3. Could replace global range set with Func Extent.
//

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <CFG.h>
#include <CodeObject.h>
#include <CodeSource.h>
#include <Function.h>
#include <Instruction.h>
#include <LineInformation.h>
#include <Module.h>
#include <Region.h>
#include <Symtab.h>

using namespace Dyninst;
using namespace ParseAPI;
using namespace SymtabAPI;
using namespace InstructionAPI;
using namespace std;

class Range;
class SymbolInfo;
class FuncInfo;
class GapInfo;
class Options;

typedef unsigned long VMA;
typedef map <VMA, Range> RangeSet;
typedef map <VMA, SymbolInfo *> SymbolMap;
typedef map <VMA, FuncInfo *> FuncMap;
typedef vector <GapInfo *> GapVector;

Symtab * the_symtab = NULL;

SymbolMap symbolMap;
FuncMap   funcMap;
GapVector gapVec;

VMA plt_start = 0;
VMA plt_end = 0;

map <int, string> typeName;

//----------------------------------------------------------------------

// One address range [start, end) with FuncInfo attribute.
class Range {
public:
    VMA  start;
    VMA  end;
    FuncInfo * finfo;

    Range(VMA st = 0, VMA en = 0, FuncInfo * f = NULL) {
	start = st;
	end = en;
	finfo = f;
    }
};

// One SymtabAPI Function symbol.  Contains one contiguous range.
class SymbolInfo {
public:
    VMA  start;
    VMA  end;
    string  linkName;
    string  prettyName;
    Module * module;

    SymbolInfo(VMA st, VMA en, string & ln, string & pr, Module * mod) {
	start = st;
	end = en;
	linkName = ln;
	prettyName = pr;
	module = mod;
    }
};

// One ParseAPI Function.  May contain multiple ranges.
class FuncInfo {
public:
    VMA  entry;
    string  name;
    ParseAPI::Function * func;
    RangeSet rset;

    FuncInfo(VMA ent, string nm, ParseAPI::Function * f) {
	entry = ent;
	name = nm;
	func = f;
	rset.clear();
    }
};

// One range from a Symtab Function, not claimed by any ParseAPI
// block.
class GapInfo {
public:
    VMA  start;
    VMA  end;
    SymbolInfo * sinfo;
    Block * prev;

    GapInfo(VMA st, VMA en, SymbolInfo * si) {
	start = st;
	end = en;
	sinfo = si;
	prev = NULL;
    }
};

// Command-line options.
class Options {
public:
    const char *filename;
    bool debug;

    Options() {
	filename = NULL;
	debug = false;
    }
};

// Order is by size of gap (higher first), and resolve ties by start
// address (lower first).
bool
gapOrder(GapInfo * x, GapInfo * y)
{
    VMA size_x = x->end - x->start;
    VMA size_y = y->end - y->start;

    return (size_x > size_y) || (size_x == size_y && x->start < y->start);
}

//----------------------------------------------------------------------

// Add one range to the range set and distinguish by FuncInfo
// attribute.
//
// Compact adjacent or overlapping ranges with the same attribute, but
// split ranges with different attributes.  If two ranges overlap with
// different attributes, then either answer is correct.
//
void
addRange(RangeSet & rset, Range range)
{
    VMA start = range.start;
    VMA end = range.end;
    FuncInfo * finfo = range.finfo;

    // find it = first range strictly right of start
    // left = range containing start, if there is one
    auto it = rset.upper_bound(start);
    auto left = it;
    if (left != rset.begin()) {
	--left;
    }

    // adjust left range if it overlaps start
    if (left != rset.end()
	&& left->second.start <= start && start <= left->second.end)
    {
	if (end <= left->second.end) {
	    // new range is a subset of left, keep the old range and
	    // discard the new one.
	    return;
	}
	if (left->second.start < start && left->second.finfo != finfo) {
	    // funcs do not match, split left range
	    left->second.end = start;
	}
	else {
	    // funcs do match, merge left with new range
	    start = std::min(start, left->second.start);
	    rset.erase(left);
	}
    }

    // delete any ranges completely contained within the new one.
    // if funcs do not match, just keep the new one.
    //
    while (it != rset.end() && it->second.end <= end) {
	auto next = it;
	++next;
	rset.erase(it);
	it = next;
    }

    // adjust right range if it overlaps end
    if (it != rset.end() && it->second.start <= end)
    {
	if (it->second.finfo == finfo) {
	    // funcs do match, merge 'it' into new range
	    end = std::max(end, it->second.end);
	    rset.erase(it);
	}
	else {
	    // funcs don't match, split 'it'
	    Range right(end, it->second.end, it->second.finfo);
	    rset.erase(it);
	    rset[end] = right;
	}
    }

    // add new range [start, end)
    rset[start] = Range(start, end, finfo);
}

//----------------------------------------------------------------------

// A Region is an ELF section that contains text (code).
void
printRegions()
{
    cout << "\n------------------------------------------------------------\n";

    vector <Region *> regVec;
    the_symtab->getAllRegions(regVec);

    for (auto rit = regVec.begin(); rit != regVec.end(); ++rit) {
	Region * reg = *rit;
	Region::RegionType type = reg->getRegionType();

	if (type == Region::RT_TEXT || type == Region::RT_TEXTDATA) {
	    Offset start = reg->getMemOffset();
	    Offset end = start + reg->getMemSize();

	    cout << "\nregion:  0x" << hex << start
		 << "--0x" << end << dec << "\n"
		 << reg->getRegionName() << "\n";
	}
    }
}

// Find the PLT region, if there is one.
void
getPLTRegion()
{
    Region * reg;
    the_symtab->findRegion(reg, ".plt");

    if (reg != NULL) {
	plt_start = reg->getMemOffset();
	plt_end = plt_start + reg->getMemSize();
    }
}

//----------------------------------------------------------------------

// A Module is one Compilation Unit (CU), one source file.
void
printModules()
{
    cout << "\n------------------------------------------------------------\n";

    vector <Module *> modvec;
    the_symtab->getAllModules(modvec);
    Module * def_mod = the_symtab->getDefaultModule();

    for (auto mit = modvec.begin(); mit != modvec.end(); ++mit) {
	Module * mod = *mit;

	cout << "\nmodule:  0x" << hex << mod->addr() << dec << "\n"
	     << ((mod == def_mod) ? "<default>" : mod->fullName())
	     << "\n";
    }
}

//----------------------------------------------------------------------

// Read SymtabAPI::Functions and make a map of SymbolInfo indexed by
// start address.
//
void
readSymbols()
{
    symbolMap.clear();

    vector <SymtabAPI::Function *> symVec;
    the_symtab->getAllFunctions(symVec);

    for (auto sit = symVec.begin(); sit != symVec.end(); ++sit) {
	SymtabAPI::Function * sym_func = *sit;
	VMA start = sym_func->getOffset();
	VMA end = start + sym_func->getSize();
	string linkName = *(sym_func->mangled_names_begin());
	string prettyName = *(sym_func->pretty_names_begin());
	Module * module = sym_func->getModule();

	auto it = symbolMap.find(start);
	if (it == symbolMap.end()) {
	    SymbolInfo * sinfo =
		new SymbolInfo(start, end, linkName, prettyName, module);
	    symbolMap[start] = sinfo;
	}
	else {
	    warnx("duplicate symbol at 0x%lx: %s", start, linkName.c_str());
	}
    }
}

void
printSymbols()
{
    cout << "\n------------------------------------------------------------\n";

    for (auto sit = symbolMap.begin(); sit != symbolMap.end(); ++sit) {
	SymbolInfo * sinfo = sit->second;

	cout << "\nsymbol:  0x" << hex << sinfo->start
	     << "--0x" << sinfo->end << dec << "\n"
	     << "link:    " << sinfo->linkName << "\n"
	     << "pretty:  " << sinfo->prettyName << "\n";
    }
}

//----------------------------------------------------------------------

// Read ParseAPI::Functions and make a map of FuncInfo indexed by
// entry address.
//
void
readFunctions(CodeObject * code_obj)
{
    funcMap.clear();

    const CodeObject::funclist & funcList = code_obj->funcs();

    for (auto fit = funcList.begin(); fit != funcList.end(); ++fit) {
	ParseAPI::Function * func = *fit;
	VMA entry = func->addr();
	string name = func->name();

	auto it = funcMap.find(entry);
	if (it != funcMap.end()) {
	    warnx("duplicate function at 0x%lx: %s", entry, name.c_str());
	    continue;
	}

	FuncInfo * finfo = new FuncInfo(entry, name, func);
	funcMap[entry] = finfo;

	// add blocks to func's range set
	const ParseAPI::Function::blocklist & blist = func->blocks();

	for (auto bit = blist.begin(); bit != blist.end(); ++bit) {
	    Block * block = *bit;
	    VMA start = block->start();
	    VMA end = block->end();

	    addRange(finfo->rset, Range(start, end, finfo));
	}
    }
}

void
printFunctions()
{
    cout << "\n------------------------------------------------------------\n";

    for (auto fit = funcMap.begin(); fit != funcMap.end(); ++fit) {
	FuncInfo * finfo = fit->second;

	cout << "\nfunc:  0x" << hex << finfo->entry << "\n"
	     << "name:  " << finfo->name << "\n";

	for (auto rit = finfo->rset.begin(); rit != finfo->rset.end(); ++rit) {
	    Range r = rit->second;
	    cout << "0x" << r.start << "--0x" << r.end << "  ";
	}
	cout << dec << "\n";
    }
}

//----------------------------------------------------------------------

// Find the first gap in [start, end) that is not contained in rset.
//
// Returns: true and sets gap_start, gap_end if there is a non-empty
// gap within the [start, end) range.
//
bool
findNextGap(RangeSet & rset, VMA start, VMA end, VMA & gap_start, VMA & gap_end)
{
    // find up = first range strictly right of start
    // left = range containing start, if there is one
    auto up = rset.upper_bound(start);
    auto left = up;
    if (left != rset.begin()) {
	--left;
    }

    if (left == rset.end() || left->second.end <= start) {
	// gap begins at start endpoint
	gap_start = start;
	gap_end = (up != rset.end() && up->second.start < end) ? up->second.start : end;

	return true;
    }

    // gap does not begin at start endpoint, find where it does begin.
    // 'it' is never end here.
    auto it = left;
    while (it->second.end <= end) {
	auto next = it;  ++next;

	if (next == rset.end() || it->second.end < next->second.start) {
	    break;
	}
	it = next;
    }

    // possible gap from it->end to next->start
    if (it->second.end < end) {
	gap_start = it->second.end;
	++it;
	gap_end = (it != rset.end() && it->second.start < end) ? it->second.start : end;

	return true;
    }

    return false;
}

// Locate all gaps and sort by size.
void
findGaps(CodeObject * code_obj)
{
    gapVec.clear();

    // global set of ranges across all functions
    RangeSet global;

    for (auto fit = funcMap.begin(); fit != funcMap.end(); ++fit) {
	FuncInfo * finfo = fit->second;

	for (auto rit = finfo->rset.begin(); rit != finfo->rset.end(); ++rit) {
	    addRange(global, rit->second);
	}
    }

    for (auto sit = symbolMap.begin(); sit != symbolMap.end(); ++sit) {
	SymbolInfo * sinfo = sit->second;
	VMA start = sinfo->start;
	VMA end = sinfo->end;

	while (start < end) {
	    VMA gap_start;
	    VMA gap_end;

	    if (findNextGap(global, start, end, gap_start, gap_end)) {
		GapInfo * gap = new GapInfo(gap_start, gap_end, sinfo);

		// find the basic block immediately before gap_start
		// and save in gap info
		VMA start_1 = gap_start - 1;
		Range rng;

		auto git = global.upper_bound(start_1);
		if (git != global.begin()) {
		    --git;
		    if (git != global.end()) {
			rng = git->second;
		    }
		}

		if (rng.start <= start_1 && start_1 < rng.end) {
		    CodeRegion * region = rng.finfo->func->region();
		    set <Block *> bset;

		    code_obj->findBlocks(region, start_1, bset);

		    if (! bset.empty()) {
			Block * blk = *(bset.begin());

			if (blk->start() <= start_1 && start_1 < blk->end()) {
			    gap->prev = blk;
			}
		    }
		}

		gapVec.push_back(gap);
		start = gap_end;
	    }
	    else {
		break;
	    }
	}
    }

    std::sort(gapVec.begin(), gapVec.end(), gapOrder);
}

//----------------------------------------------------------------------

// Fill in edge type names.
void
initTypeNames()
{
    typeName[CALL] = "call";
    typeName[COND_TAKEN] = "cond-branch";
    typeName[COND_NOT_TAKEN] = "cond-branch";
    typeName[INDIRECT] = "indirect-branch";
    typeName[DIRECT] = "branch";
    typeName[FALLTHROUGH] = "fallthrough";
    typeName[CALL_FT] = "fallthrough";
    typeName[CATCH] = "catch";
    typeName[RET] = "return";
}

// Returns: label for edge type: call, cond, sink, etc.
//
// If the edge is call to no-return function, then set noRetName to
// the function name.
//
string
edgeType(Edge * edge, string & noRetName)
{
    if (edge == NULL) {
	return "null";
    }

    if (edge->sinkEdge()) {
	return "sink";
    }

    // call to no-return function is special
    if (edge->type() == CALL) {
	Block * targ = edge->trg();
	vector <ParseAPI::Function *> flist;
	targ->getFuncs(flist);

	if (! flist.empty()) {
	    ParseAPI::Function * func = *(flist.begin());

	    if (func->retstatus() == NORETURN) {
		noRetName = func->name();
		return "call-noreturn";
	    }
	}
    }

    auto it = typeName.find(edge->type());
    if (it != typeName.end()) {
	return it->second;
    }

    return "unknown";
}

//----------------------------------------------------------------------

// Analyze each gap for heuristic features to guess if it is a missing
// code range.
//
//  1. size of gap -- larger is more suspicious.
//  2. location of gap -- end of function is less suspicious.
//  3. is gap in PLT region -- if yes, this is less suspicious.
//  4. out edges from block before gap -- sink is suspicious.
//  5. line map info -- if exists, this is very suspicious.
//
void
analyzeGap(GapInfo * ginfo)
{
    SymbolInfo * sinfo = ginfo->sinfo;
    Module * module = sinfo->module;
    VMA start = ginfo->start;
    VMA end = ginfo->end;

    // 1 -- size of gap, larger is more suspicious
    cout << "\ngap:       0x" << hex << start << "--0x" << end << dec
	 << "  (len: " << (end - start) << ")\n"
	 << "name:      " << sinfo->linkName << "\n";

    // 2 -- location of gap within the symbol range, at the end is
    // less suspicious
    string loc;
    if (start <= sinfo->start && sinfo->end <= end) {
	loc = "entire function";
    }
    else if (start <= sinfo->start) {
	loc = "start";
    }
    else if (sinfo->end <= end) {
	loc = "end";
    }
    else {
	loc = "middle";
    }
    cout << "location:  " << loc;

    // 3 -- is gap inside PLT region
    if (plt_start <= start && start < plt_end) {
	cout << "  (plt)";
    }
    cout << "\n";

    // 4 -- types of outgoing edges from previous block
    Block * prev = ginfo->prev;
    VMA start_1 = start - 1;
    string noRetName("");

    cout << "prev blk:  ";
    if (prev == NULL
	|| ! (prev->start() <= start_1 && start_1 < prev->end()))
    {
	cout << "no block";
    }
    else {
	const Block::edgelist & elist = prev->targets();

	if (elist.empty()) {
	    cout << "no outedges";
	}
	else {
	    for (auto eit = elist.begin(); eit != elist.end(); ++eit) {
		if (eit != elist.begin()) {
		    cout << ",  ";
		}
		cout << edgeType(*eit, noRetName);
	    }
	}
    }
    cout << "\n";

    // name of no-return function, if any
    if (noRetName != "") {
	cout << "no-ret:    " << noRetName << "\n";
    }

    // 5 -- line map info.  if exists, this is very suspicious.
    // try every 4 or 1 byes in gap
    string file;
    int line = 0;
    int inc = (end - start > 95) ? 4 : 1;
    int num = 0;
    int num_yes = 0;

    for (VMA addr = start; addr < end; addr += inc) {
	vector <Statement *> svec;
	module->getSourceLines(svec, addr);

	num++;
	if (! svec.empty()) {
	    if (line == 0) {
		file = svec[0]->getFile();
		line = svec[0]->getLine();
	    }
	    num_yes++;
	}
    }
    cout << "line map:  " << ((num_yes > 0) ? "yes" : "no")
	 << "  (" << num_yes << " of " << num << ")\n";

    if (line > 0) {
	cout << "file:      " << file << "\n"
	     << "line:      " << line << "\n";
    }
}

void
printGaps(bool debug)
{
    if (debug) {
	cout << "\n------------------------------------------------------------\n";
    }

    long num_gaps = 0;
    long sum_size = 0;

    for (auto git = gapVec.begin(); git != gapVec.end(); ++git) {
	GapInfo * ginfo = *git;

	analyzeGap(ginfo);
	num_gaps++;
	sum_size += ginfo->end - ginfo->start;
    }

    cout << "\nnum gaps: " << num_gaps
	 << "   total size: " << sum_size << "\n\n";
}

//----------------------------------------------------------------------

void
usage(string mesg)
{
    if (! mesg.empty()) {
        cout << "error: " << mesg << "\n";
    }
    cout << "usage: find-gaps [options]... filename\n\n"
	 << "options:\n"
	 << "  -D    display info on regions, modules, symbols and function\n"
	 << "        ranges for debugging\n"
	 << "\n";

    exit(1);
}

// Command-line options
void
getOptions(int argc, char **argv, Options & opts)
{
    if (argc < 2) {
	usage("");
    }

    int n = 1;
    while (n < argc) {
	string arg(argv[n]);

	if (arg == "-D") {
	    opts.debug = true;
	    n++;
	}
	else {
	    break;
	}
    }

    if (n < argc) {
	opts.filename = argv[n];
    }
    else {
	usage("missing file name");
    }
}

//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
    Options opts;

    getOptions(argc, argv, opts);

    if (! Symtab::openFile(the_symtab, opts.filename)) {
        errx(1, "Symtab::openFile failed: %s", opts.filename);
    }

    initTypeNames();

    // SymtabAPI Function symbols
    the_symtab->parseTypesNow();
    the_symtab->parseFunctionRanges();

    vector <Module *> modVec;
    the_symtab->getAllModules(modVec);

    for (auto mit = modVec.begin(); mit != modVec.end(); ++mit) {
	(*mit)->parseLineInformation();
    }

    readSymbols();
    getPLTRegion();

    if (opts.debug) {
	printRegions();
	printModules();
	printSymbols();
    }

    // ParseAPI Function ranges
    SymtabCodeSource * code_src = new SymtabCodeSource(the_symtab);
    CodeObject * code_obj = new CodeObject(code_src);
    code_obj->parse();

    readFunctions(code_obj);

    if (opts.debug) {
	printFunctions();
    }

    // analyze both sets of ranges for gaps
    findGaps(code_obj);
    printGaps(opts.debug);

    return 0;
}
