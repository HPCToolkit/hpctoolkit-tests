//
//  Copyright (c) 2016, Rice University.
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
//----------------------------------------------------------------------
//
//  Acceptance test to see if a compiler is generating proper debug
//  info for inline code and C++ templates.  We use Dyninst ParseAPI
//  and SymtabAPI to iterate the functions and machine instructions
//  and read the dwarf info.
//
//  John Mellor-Crummey
//  Rice University
//  July 2017
//  johnmc@rice.edu
//
//  Usage: ./symbols <load-module>
//

//******************************************************************************
// include files
//******************************************************************************

#include <stdio.h>
#include <stdlib.h>

#include <err.h>

#include <map>
#include <set>

#include <Symtab.h>
#include <Function.h>


//******************************************************************************
// name spaces
//******************************************************************************

using namespace Dyninst;
// using namespace ParseAPI;
using namespace SymtabAPI;

using namespace std;



//******************************************************************************
// types
//******************************************************************************

typedef std::set<string> NameSet;

typedef std::map<Offset, NameSet *> FunctionEndpoints;



//******************************************************************************
// global variables 
//******************************************************************************

Symtab * the_symtab = NULL;

FunctionEndpoints functionEndpoints;


//******************************************************************************
// functions 
//******************************************************************************

static void 
addFunction(Offset offset, string name)
{
  auto it = functionEndpoints.find(offset);
  if (it != functionEndpoints.end()) {
    it->second->insert(name);
  } else {
    NameSet *n = new NameSet;
    n->insert(name);
    functionEndpoints[offset] = n;
  }
}


static void
dumpFunctions()
{
  for (auto it=functionEndpoints.begin(); it!=functionEndpoints.end(); ++it) {
    cout << "0x" << hex << it->first << dec << "    ";
    bool first = true;
    for (auto names=it->second->begin(); names != it->second->end(); ++names) {
      string name = *names;
      if (first) {
	first = false;
      } else {
	cout << ", ";
      }
      cout << name;
    }
    cout << "\n";
  }
}

static void 
addSymbols()
{
    vector<Dyninst::SymtabAPI::Symbol *> symbols;
    the_symtab->getAllSymbolsByType(symbols, Symbol::ST_FUNCTION);
    for (int i = 0; i < symbols.size(); i++) {
      Symbol *s = symbols[i];
      string mname = s->getMangledName();
      Offset o = s->getOffset();
      addFunction(o, mname);
    }
}


static void 
addSymtabFunctions()
{
    vector<Dyninst::SymtabAPI::Function *> symtabFunctions;
    the_symtab->getAllFunctions(symtabFunctions);
    for (int i = 0; i < symtabFunctions.size(); i++) {
      Dyninst::SymtabAPI::Function *f = symtabFunctions[i];
      Offset o = f->getOffset();
      string names;
      bool first = true;
      for (auto n = f->mangled_names_begin(); n != f->mangled_names_end(); n++) {
	addFunction(o, *n);
      }
    }
}

int
main(int argc, char **argv)
{
    if (argc != 2) {
      errx(1, "usage: symbols <load-module>");
    }

    char *filename = argv[1];

    // open symtab
    if (! Symtab::openFile(the_symtab, filename)) {
	errx(1, "Symtab::openFile failed: %s", filename);
    }
    the_symtab->parseTypesNow();

    // scan the load module's function symbols
    addSymbols();

    // output the address and associated names of the 
    // load module's function symbols 
    dumpFunctions();

    return 0;
}
