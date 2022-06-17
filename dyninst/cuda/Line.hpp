// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// --------------------------------------------------------------------------
// Part of HPCToolkit (hpctoolkit.org)
//
// Information about sources of support for research and development of
// HPCToolkit is at 'hpctoolkit.org' and in 'README.Acknowledgments'.
// --------------------------------------------------------------------------
//
// Copyright ((c)) 2002-2018, Rice University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage.
//
// ******************************************************* EndRiceCopyright *


//***************************************************************************
//
// File: Line.hpp
//
// Purpose:
//   Interface for a Line Map Line 
//
//***************************************************************************

#ifndef __Line_hpp__
#define __Line_hpp__

//******************************************************************************
// system includes
//******************************************************************************

#include <inttypes.h>
#include <linux/limits.h>
#include <vector>




//******************************************************************************
// forward declarations
//******************************************************************************

class LineDecoderSettings;



//******************************************************************************
// type declarations
//******************************************************************************

class LineInfo {
public:
  uint64_t address;           // address of line
  uint8_t op_index;           // index in VLIW inst; 0 if non VLIW
  uint16_t file;              // file table entry
  uint32_t line;              // source line number; 0 = unknown
  uint32_t column;            // column within source line; 0 = unknown

  bool is_stmt;               // recommended breakpoint location
  bool basic_block;           // current inst is the beginning of a basic block
  bool end_sequence;          // first byte after sequence of inst; resets row
  bool prologue_end;          // point for an entry breakpoint of a function
  bool epilogue_begin;        // point for an exit breakpoint of a function

  uint32_t isa;               // encodes the applicable inst set arch
  uint32_t discriminator;     // identifies the block of the current inst 

public:

  void setAddress(uint64_t a)         { address        = a;   op_index = 0;  };
  void fixedAdvancePC(uint64_t o)     { address        += o;  op_index = 0;  };

  void setFile(uint64_t f)            { file           = f;                  };

  void setLine(uint64_t l)            { line           = l;                  };
  void advanceLine(uint64_t o)        { line           += o;                 };

  void setColumn(uint64_t c)          { column         = c;                  };
  void negateStmt()                   { is_stmt        = !is_stmt;           };

  void setIsa(uint32_t l)             { isa            = l;                  };

  void basicBlock()                   { basic_block    = true;               };
  void endSequence()                  { end_sequence   = true;               };
  void endPrologue()                  { prologue_end   = true;               };
  void beginEpilogue()                { epilogue_begin = true;               };
  void setDiscrim(uint32_t d)         { discriminator  = d;                  };

  void reset(const LineDecoderSettings &lds);
  void resetFlagsAndDiscriminator();

  void advancePC
  (
   uint64_t operation_advance,
   const LineDecoderSettings &lds 
  );

  void applySpecialOp
  (
   uint64_t opcode_raw,
   const LineDecoderSettings &lds
  );
};


class FileSystemRepr;


class FileSystem {
public:

  FileSystem();
  ~FileSystem();

  const char *getFileName(int i);
  const char *getDirName(int i);


public:
   FileSystemRepr *repr;
};


class LineInfoHandler {
public:
  virtual void processMatrixRow
  (
    LineInfo *li, 
    FileSystem *fs
  ) { 
  }; 
};

#endif
