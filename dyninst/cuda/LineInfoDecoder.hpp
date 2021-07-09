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
// File: LineInfoDecoder.hpp
//
// Purpose:
//   Interface for a LineInfoDecoder
//
//***************************************************************************

#ifndef __LineInfoDecoder_hpp__
#define __LineInfoDecoder_hpp__

#include "Line.hpp"

class LineInfoDecoder : public LineInfoHandler {
public:
  LineInfoDecoder() {
    start_address = 0;
    end_address = 0;
    file_number = 0;
    line = 0;
    column = 0;
    first = true;
  };

  void reportLine(FileSystem *fs) {
    reportLine(start_address, end_address, file_number, line, column, fs); 
  };


  //****************************************************************************
  // virtual functions
  //****************************************************************************

  virtual void reportLine
  (
    uint64_t start_address,
    uint64_t end_address,
    uint16_t file_number,
    uint32_t line, 
    uint32_t column,
    FileSystem *fs
  ) {
  };


  virtual void processMatrixRow
  (
    LineInfo *li,
    FileSystem *fs
  ) {
    if (first) {
      first = false;
    } else {
      end_address = li->address;
      reportLine(fs);
    }

    // set state for next line
    file_number = li->file;
    line = li->line;
    column = li->column;
    start_address = li->address;
  };


private:
  uint64_t start_address;     // start address of line
  uint64_t end_address;       // end address of line
  uint16_t file_number;       // file table entry
  uint32_t line;              // source line number; 0 = unknown
  uint32_t column;            // column within source line; 0 = unknown

protected:
  bool first;
};

#endif
