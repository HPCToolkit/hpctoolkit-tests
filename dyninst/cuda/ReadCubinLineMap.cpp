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
// File: ReadLineMap.cpp
//
// Purpose:
//   Read line map of cubin. 
//
//***************************************************************************

//******************************************************************************
// system includes
//******************************************************************************

#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include <iostream>
#include <iomanip>
#include <vector>

#include <dwarf.h>



//******************************************************************************
// local includes
//******************************************************************************

#include "ElfHelper.hpp"
#include "Line.hpp"


//******************************************************************************
// macros
//******************************************************************************

#define DEBUG_LINE_SECTION_NAME ".debug_line"

#define need(p,k,e) if (p + k > e) return false;

#define V(x) ((void*) x)


#if DUMP_RAW_LINE
#define DUMP_RAW_OUTPUT(...) std::cout << __VA_ARGS__
#else
#define DUMP_RAW_OUTPUT(...)
#endif


#define DUMP_HEADER       1


#if DUMP_HEADER 
#define DUMP_HEADER_OUTPUT(...) std::cout << __VA_ARGS__
#else
#define DUMP_HEADER_OUTPUT(...)
#endif



//******************************************************************************
// type definitions
//******************************************************************************

typedef std::vector<Elf_Scn *> Elf_SectionVector;
typedef std::vector<Elf64_Addr> Elf_SymbolVector;

class DirectoryTable : public std::vector<const char *> {
public:
  const void *offset;

  DirectoryTable() { push_back(NULL); };

  const char *
  entry
  (
    int i
  ) 
  { 
    const char **str = this->data();
    const char *result = NULL;
    if (i > 0 && i < size()) result = str[i];
    return result;
  }

  void dump() {
    const char **str = this->data();
    for (int i = 1; i < size(); i++) {
      DUMP_HEADER_OUTPUT("  " << i << "\t" << str[i] << "\n");
    }
  };
};


class FileTableEntry {
public:
  uint16_t dir_index;
  uint64_t time;
  uint64_t size;
  const char *filename;

  FileTableEntry
  (
   const unsigned char *&ptr
  );
  
  const char *getFileName() { return filename; };

  uint16_t getDirIndex() { return dir_index; };

  void dump
  (
    int i
  ) {
    DUMP_HEADER_OUTPUT("  " << i << "\t" << dir_index << "\t" << time <<
		 "\t" << size << "\t" << filename << "\n");
  };
};


class FileTable : public std::vector<FileTableEntry *> {
public:
  FileTable() { push_back(NULL); };


  const char *getFileName
  (
    int i
  ) {
    FileTableEntry **fte = this->data();
    return fte[i]->getFileName(); 
  }; 


  uint16_t 
  getDirIndex
  (
    int i
  ) {
    FileTableEntry **fte = this->data();
    return fte[i]->getDirIndex(); 
  };

  void dump() {
    FileTableEntry **fte = this->data();
    for (int i = 1; i < size(); i++) {
      fte[i]->dump(i);
    }
  };

public:
  const void *offset;
};


class FileSystemRepr {
public:
  DirectoryTable &getDirectoryTable() { return directories; }

  FileTable &getFileTable() { return files; }

public:
  DirectoryTable directories;
  FileTable files;
};

class LineDecoderSettings {
public:
  uint8_t min_inst_len;
  uint8_t max_ops_per_inst;
  uint8_t default_is_stmt;
  int8_t  line_base;
  uint8_t line_range;
  uint8_t opcode_base;

  void read
  (
   uint16_t dwarf_version,
   const unsigned char *&ptr
  );
};


class OpcodeLengthTable : public std::vector<uint16_t> {
public:
  OpcodeLengthTable() { push_back(0); };

  void dump() {
    uint16_t *len = this->data();
    for (int i = 1; i < size(); i++) {
      DUMP_HEADER_OUTPUT("  Opcode " << i << " has " << len[i]); 
      if (len[i] == 1) DUMP_HEADER_OUTPUT(" arg\n"); 
      else DUMP_HEADER_OUTPUT(" args\n"); 
    }
  };
};


class LineMapInfo {
public:
  uint64_t unit_length;
  uint64_t hdr_length;
  uint16_t dwarf_version;

  LineDecoderSettings lds;
  OpcodeLengthTable op_lengths; 
  FileSystem fs;

  bool parse
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *start,
   const unsigned char *end,
   const unsigned char **sptr,
   LineInfoHandler *lih
  );

  void dump();

private:

  bool parseHeader
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *end,
   const unsigned char **sptr
  );

  bool parseOpcodeLengthTable
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *end,
   const unsigned char **sptr
  );

  bool parseDirectoryTable
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *start,
   const unsigned char *end,
   const unsigned char **sptr
  );

  bool parseFileTable
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *start,
   const unsigned char *end,
   const unsigned char **sptr
  );

  bool parseLineMap
  (
   GElf_Ehdr *ehdr,
   Elf_Scn *scn,
   GElf_Shdr *shdr,
   const unsigned char *start,
   const unsigned char *end,
   const unsigned char **sptr,
   LineInfoHandler *lih
  );

  void dumpHeader();
  void dumpOpcodeTable();
  void dumpDirectoryTable();
  void dumpFileTable();
}; 



//******************************************************************************
// private functions
//******************************************************************************

// form an unsigned integer by reading 'len' bytes in little endian order
static uint64_t
uread(const unsigned char *&p, unsigned int len)
{
  uint64_t val = 0;
  uint8_t o = 0;
  switch (len) {
  case 8: val |= ((uint64_t) *p++); o+=8;
  case 7: val |= ((uint64_t) *p++) << o; o+=8;
  case 6: val |= ((uint64_t) *p++) << o; o+=8;
  case 5: val |= ((uint64_t) *p++) << o; o+=8;
  case 4: val |= ((uint64_t) *p++) << o; o+=8;
  case 3: val |= ((uint64_t) *p++) << o; o+=8;
  case 2: val |= ((uint64_t) *p++) << o; o+=8;
  case 1: val |= ((uint64_t) *p++) << o;
    break;
  }
  return val;
}


// read an unsigned LEB128 encoded integer
// return the result and advance ptr past the data
static uint64_t
uread_leb128(const unsigned char *&ptr)
{
  uint64_t result = 0;
  uint8_t pos = 0; // bits from the first byte belong in the lowest result bits
  unsigned char byte;
  for (;;) {
    byte = *ptr++;
    result |= ((byte & 0x7f) << pos); // put bottom 7 bits of byte in place
    if ((byte & 0x80) == 0) break; // no more bytes remain
    pos += 7; // compute next insertion point
  }
  return result;
}


// read a signed LEB128 encoded integer
// return the result and advance ptr past the data
static int64_t
sread_leb128(const unsigned char *&ptr)
{
  uint64_t result = 0;
  uint8_t pos = 0; // bits from the first byte belong in the lowest result bits
  uint8_t nbits = sizeof(result) << 3;
  unsigned char byte;
  for (;;) {
    byte = *ptr++;
    result |= ((byte & 0x7f) << pos); // put bottom 7 bits of byte in place
    pos += 7; // compute next insertion point
    if ((byte & 0x80) == 0) break; // no more bytes remain
  }

  if (byte & 0x40) { // check sign bit of last byte 
    // result must be negative
    if (pos < nbits) { // sign needs extension
      result |= (~0 << pos); // extend sign bit
    }
  }
  return (int64_t) result;
}


FileSystem::FileSystem()
{
  repr = new FileSystemRepr;
}


FileSystem::~FileSystem()
{
  delete repr;
}

const char *
FileSystem::getFileName(int i)
{
  return repr->files.getFileName(i);
}


const char *
FileSystem::getDirName(int i)
{
  uint16_t index = repr->files.getDirIndex(i);
  return repr->directories.entry(index);
}


FileTableEntry::FileTableEntry
(
 const unsigned char *&ptr
)
{
  // get file name
  filename = (const char *) ptr;
  while (*ptr++ != 0); // advance to end of current string
  
  // read directory index
  dir_index = uread_leb128(ptr);
  
  // skip modification time
  time = uread_leb128(ptr);
  
  // skip file length 
  size = uread_leb128(ptr);
}



void
LineDecoderSettings::read
(
 uint16_t dwarf_version,
 const unsigned char *&ptr
)
{
  // read minimum instruction length
  min_inst_len = uread(ptr, 1);
  
  // read maximum operations per instruction if version 4+; else assume 1 
  max_ops_per_inst = dwarf_version < 4 ? 1 : uread(ptr, 1);

  // read default value of is_stmt
  default_is_stmt = uread(ptr, 1);

  // read line base 
  line_base = *(const char *)ptr++; // signed read of one byte

  // read line range
  line_range = uread(ptr, 1);

  // read opcode base
  opcode_base = uread(ptr, 1);
}
    

void
LineInfo::reset
(
  const LineDecoderSettings &lds
)
{
  address = 0;
  op_index = 0;
  file = 1;
  line = 1;
  column = 0;
  is_stmt = lds.default_is_stmt;
  isa = 0;

  resetFlagsAndDiscriminator();
}

void
LineInfo::resetFlagsAndDiscriminator
(
)
{
  basic_block = false;
  end_sequence = false;
  prologue_end = false;
  epilogue_begin = false;
  discriminator = 0;
}
      


void 
LineInfo::advancePC
(
 uint64_t operation_advance,
 const LineDecoderSettings &lds
)
{									
  int sum = op_index + operation_advance;
  int div = sum / lds.max_ops_per_inst;
  int rem = sum % lds.max_ops_per_inst;

  address += lds.min_inst_len * div;
  op_index = rem;
}


void 
LineInfo::applySpecialOp
(
 uint64_t opcode,
 const LineDecoderSettings &lds
)
{									
  uint64_t special_opcode = opcode - lds.opcode_base;
  line += lds.line_base + (special_opcode % lds.line_range);

  uint64_t operation_advance = special_opcode / lds.line_range;
  advancePC(operation_advance, lds);
}


bool
LineMapInfo::parse
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *start,
 const unsigned char *end,
 const unsigned char **sptr,
 LineInfoHandler *lih
)
{
  parseHeader(ehdr, scn, shdr, end, sptr);
  parseOpcodeLengthTable(ehdr, scn, shdr, end, sptr);
  parseDirectoryTable(ehdr, scn, shdr, start, end, sptr);
  parseFileTable(ehdr, scn, shdr, start, end, sptr);
  dump();
  parseLineMap(ehdr, scn, shdr, start, end, sptr, lih);
}


bool
LineMapInfo::parseHeader
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *end,
 const unsigned char **sptr
)
{
  const unsigned char *ptr = *sptr;
  int hdr_length_bytes = 4;

  // if not 4 bytes available, return false
  need(ptr, 4, end);

  // read unit length 
  {
    unit_length = uread(ptr, 4);
    
    if (unit_length == 0xffffffff) {
      // if not 8 bytes available, return false
      need(ptr, 8, end);
      
      unit_length = uread(ptr, 8);
      hdr_length_bytes = 8;
    }
  }

  // read DWARF version
  dwarf_version = uread(ptr, 2);

  // read header length
  hdr_length = uread(ptr, hdr_length_bytes);

  lds.read(dwarf_version, ptr);

  // return updated pointer into the section
  *sptr = ptr;

  return true;
}


bool
LineMapInfo::parseOpcodeLengthTable
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *end,
 const unsigned char **sptr
)
{
  const unsigned char *ptr = *sptr;

  // read the table of opcode lengths
  for(int i = 0; i < lds.opcode_base - 1; i++) {
    op_lengths.push_back((uint16_t) uread(ptr, 1));
  }

  // return updated pointer into the section
  *sptr = ptr;

  return true;
}


bool
LineMapInfo::parseDirectoryTable
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *start,
 const unsigned char *end,
 const unsigned char **sptr
)
{
  const char *ptr = (const char *) *sptr;

  DirectoryTable &directories = fs.repr->getDirectoryTable();

  // the directory table is a sequence of strings ending with a empty string

  directories.offset = (void *) (*sptr - start);
  
  // while not an empty string
  while (*ptr != 0) {
    directories.push_back(ptr);
    while (*ptr++ != 0); // advance to end of current string
  }
  
  // advance past final empty string
  ptr++;
  
  // return updated pointer into the section
  *sptr = (const unsigned char *) ptr;

  return true;
}

bool
LineMapInfo::parseFileTable
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *start,
 const unsigned char *end,
 const unsigned char **sptr
)
{
  const unsigned char *ptr = (const unsigned char *) *sptr;

  FileTable &files = fs.repr->getFileTable();

  // the file table is a sequence of strings ending with a empty string

  files.offset = (void *) (*sptr - start);

  // while not an empty string
  while (*ptr != 0) {
    files.push_back(new FileTableEntry(ptr));
  }
  
  // advance past final empty string
  ptr++;
  
  // return updated pointer into the section
  *sptr = ptr;

  return true;
}


bool
LineMapInfo::parseLineMap
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 const unsigned char *start,
 const unsigned char *end,
 const unsigned char **sptr,
 LineInfoHandler *lih
)
{
  FileTable &files = fs.repr->getFileTable();
  DirectoryTable &directories = fs.repr->getDirectoryTable();

  const unsigned char *ptr = *sptr;

  LineInfo lineInfo;

  lineInfo.reset(lds);

  if (ptr == end) {
    DUMP_RAW_OUTPUT(" No Line Number Statements.\n");
  } else {
    DUMP_RAW_OUTPUT(" Line Number Statements:\n");

    while (ptr < end) {
      size_t offset = ptr - start;

      // read the opcode
      unsigned int opcode = uread(ptr, 1);

      DUMP_RAW_OUTPUT("  [" <<
		   std::internal << std::hex << std::setw(10) <<
		   std::setfill('0') << V(offset) << std::dec <<
		   "]  "); 

      if (opcode >= lds.opcode_base) {

	// special opcode

	uint64_t prev_address = lineInfo.address;
	uint64_t prev_line = lineInfo.line;

	lineInfo.applySpecialOp(opcode, lds);

	unsigned int addr_offset = lineInfo.address - prev_address;
	int line_offset = lineInfo.line - prev_line;

	DUMP_RAW_OUTPUT(
	  "Special opcode "  << opcode - lds.opcode_base <<
	  ": advance Address by " << addr_offset << " to " <<
	  std::hex << (void *) lineInfo.address << std::dec);

	if (lineInfo.op_index)
	  DUMP_RAW_OUTPUT(", op_index = " << (uint16_t) lineInfo.op_index);

	DUMP_RAW_OUTPUT(" and Line by " << line_offset << " to " <<
		     lineInfo.line << "\n");

	// append row to matrix
        lih->processMatrixRow(&lineInfo, &fs);

        lineInfo.resetFlagsAndDiscriminator();

      } else if (opcode == 0) {

	// read length
	unsigned int len = uread_leb128(ptr);

	const unsigned char *inst_begin = ptr;

	// read sub-opcode
	opcode = uread(ptr, 1);

	DUMP_RAW_OUTPUT("Extended opcode "  << opcode << ": ");
      
	switch (opcode) {

	case DW_LNE_end_sequence:
          lineInfo.endSequence();

	  DUMP_RAW_OUTPUT("End of Sequence\n\n");

	  // append row to matrix 
          lih->processMatrixRow(&lineInfo, &fs);

	  lineInfo.reset(lds); // reset line to defaults

	  break;

	case DW_LNE_set_address: {
	  uint64_t addr = uread(ptr, len - (ptr - inst_begin));
	  lineInfo.setAddress(addr);

	  DUMP_RAW_OUTPUT("set Address to "); 

	  if (addr == 0) 
	    DUMP_RAW_OUTPUT("0x0\n"); // the line below drops the 0x for 0 
	  else
	    DUMP_RAW_OUTPUT(std::hex << (void *) addr << std::dec << "\n");

	  break;
	}
	
	case DW_LNE_define_file: {
	  FileTableEntry *fte = new FileTableEntry(ptr);
	  files[fte->dir_index] = fte;

	  DUMP_RAW_OUTPUT("define new file: dir=" << fte->dir_index <<
		       ", name=" << fte->filename << "\n");

	  break;
	}
	
	case DW_LNE_set_discriminator: {
	  uint64_t dis = uread_leb128(ptr);

	  lineInfo.setDiscrim(dis);

	  DUMP_RAW_OUTPUT("set discriminator to " << dis << "\n");

	  break;
	}
	
	default:
	  ptr += len - 1;

	  DUMP_RAW_OUTPUT("unknown opcode\n");

	  break;
	}

      } else if (opcode <= DW_LNS_set_isa) {

	// standard opcode

	switch (opcode) {

	case DW_LNS_advance_line: {
	  int64_t offset = sread_leb128(ptr);
	  lineInfo.advanceLine(offset);

	  DUMP_RAW_OUTPUT("Advance Line by " <<
		       offset << " to " << lineInfo.line << "\n");

	  break;
	}

	case DW_LNS_advance_pc: {
	  uint64_t offset = uread_leb128(ptr);
	  lineInfo.advancePC(offset, lds);

	  DUMP_RAW_OUTPUT("Advance PC by " <<
		       offset << " to " << V(lineInfo.address) << "\n");

	  break;
	}

	case DW_LNS_const_add_pc: {
	  int offset = 255 - lds.opcode_base;
	  int div = offset / lds.line_range;
	  lineInfo.advancePC(div, lds);

	  DUMP_RAW_OUTPUT("Advance address by constant " <<
		       offset << " to " << V(lineInfo.address));

	  if (lineInfo.op_index)
	    DUMP_RAW_OUTPUT(", op_index to " << lineInfo.op_index);

	  DUMP_RAW_OUTPUT("\n");

	  break;
	}
	
	case DW_LNS_copy:

	  DUMP_RAW_OUTPUT("Copy\n");

	  // append row to matrix
          lih->processMatrixRow(&lineInfo, &fs);

          lineInfo.resetFlagsAndDiscriminator();

	  break;

	case DW_LNS_fixed_advance_pc: {
	  unsigned int offset = uread(ptr, 2);
	  lineInfo.fixedAdvancePC(offset);

	  DUMP_RAW_OUTPUT("advance address by fixed value " << offset <<
		       " to " << V(lineInfo.address));

	  break;
	}

	case DW_LNS_negate_stmt:
	  lineInfo.negateStmt();

	  DUMP_RAW_OUTPUT("set 'is_stmt' to " << lineInfo.is_stmt << "\n");

	  break;
	
	case DW_LNS_set_basic_block:
	  lineInfo.basicBlock();

	  DUMP_RAW_OUTPUT("set basic block flag\n");

	  break;
	
	case DW_LNS_set_column: {
	  unsigned int col = uread_leb128(ptr);
	  lineInfo.setColumn(col);

	  DUMP_RAW_OUTPUT("Set Column to " << col << "\n");

	  break;
	}
	
	case DW_LNS_set_epilogue_begin:
	  lineInfo.beginEpilogue();

	  DUMP_RAW_OUTPUT("Set epilogue begin flag\n");

	  break;
	
	case DW_LNS_set_file: {
	  uint64_t file = uread_leb128(ptr);
	  lineInfo.setFile(file);

	  DUMP_RAW_OUTPUT("Set File Name to entry " << file <<
		       " in the File Name Table\n");

	  break;
	}
	case DW_LNS_set_isa: {
	  unsigned int isa = uread_leb128(ptr);
	  lineInfo.setIsa(isa);

	  DUMP_RAW_OUTPUT("set isa to " << isa << "\n");
	  break;
	}

	case DW_LNS_set_prologue_end:
	  lineInfo.endPrologue();
	  
	  DUMP_RAW_OUTPUT("Set prologue end flag\n");
	  break;
	}
      }
    }

    // return updated pointer into the section
    *sptr = (const unsigned char *) ptr;

    DUMP_RAW_OUTPUT("\n");
  }

  return true;
}


static void
parseLineSection
(
 GElf_Ehdr *ehdr,
 Elf_Scn *scn,
 GElf_Shdr *shdr,
 LineInfoHandler *lih
)
{
  if (shdr->sh_size == 0) return;

  Elf_Data *data = elf_getdata(scn, NULL);

  if (data == NULL) return;
  
  const unsigned char *start = (const unsigned char *) data->d_buf;
  const unsigned char *ptr = start;

  if (start == NULL) return;

  const unsigned char *end = start + data->d_size;

  LineMapInfo lmh;
  lmh.parse(ehdr, scn, shdr, start, end, &ptr, lih);
}


void
LineMapInfo::dumpHeader
(
)
{
  DUMP_HEADER_OUTPUT
    ("Raw dump of debug contents of section .debug_line:\n\n" <<
     "  Offset:                      0x0\n" <<
     "  Length:                      " <<          unit_length      << "\n" <<
     "  DWARF Version:               " <<          dwarf_version    << "\n" <<
     "  Prologue Length:             " <<          hdr_length       << "\n" <<
     "  Minimum Instruction Length:  " <<(uint16_t)lds.min_inst_len << "\n" <<
     "  Initial value of 'is_stmt':  " <<(uint16_t)lds.default_is_stmt<<"\n"<<
     "  Line Base:                   " <<(int16_t) lds.line_base    << "\n" <<
     "  Line Range:                  " <<(uint16_t)lds.line_range   << "\n" <<
     "  Opcode Base:                 " <<(uint16_t)lds.opcode_base  << "\n\n"
    );
}


void
LineMapInfo::dumpOpcodeTable
(
)
{
  DUMP_HEADER_OUTPUT(" Opcodes:\n");
  op_lengths.dump();
  DUMP_HEADER_OUTPUT("\n");
}


void
LineMapInfo::dumpDirectoryTable
(
)
{
  DirectoryTable &directories = fs.repr->getDirectoryTable();

  DUMP_HEADER_OUTPUT(" The Directory Table (offset " <<
	       std::hex << directories.offset << std::dec << "):\n");
  directories.dump();
  DUMP_HEADER_OUTPUT("\n");
}


void
LineMapInfo::dumpFileTable
(
)
{
  FileTable &files = fs.repr->getFileTable();

  DUMP_HEADER_OUTPUT(" The File Name Table (offset " <<
	       std::hex << files.offset << std::dec << "):\n"
	       "  Entry\tDir\tTime\tSize\tName\n");
  files.dump();
  DUMP_HEADER_OUTPUT("\n");
}


void
LineMapInfo::dump
(
)
{
  dumpHeader();
  dumpOpcodeTable();
  dumpDirectoryTable();
  dumpFileTable();
}


// if the cubin contains a line map section and a matching line map relocations
// section, apply the relocations to the line map
static void
readLineMap
(
 char *cubin_ptr,
 Elf *elf,
 Elf_SectionVector *sections,
 LineInfoHandler *lih
)
{
  GElf_Ehdr ehdr_v;
  GElf_Ehdr *ehdr = gelf_getehdr(elf, &ehdr_v);
  if (ehdr) {
    unsigned line_map_scn_index;
    char *line_map = NULL;

    //-------------------------------------------------------------
    // scan through the sections to locate a line map, if any
    //-------------------------------------------------------------
    int index = 0;
    for (auto si = sections->begin(); si != sections->end(); si++, index++) {
      Elf_Scn *scn = *si;
      GElf_Shdr shdr;
      if (!gelf_getshdr(scn, &shdr)) continue;
      if (shdr.sh_type == SHT_PROGBITS) {
	const char *section_name =
	  elf_strptr(elf, ehdr->e_shstrndx, shdr.sh_name);
	if (strcmp(section_name, DEBUG_LINE_SECTION_NAME) == 0) {
	  // remember the index of line map section. we need this index to find
	  // the corresponding relocation section.
	  line_map_scn_index = index;

	  // compute line map position from start of cubin and the offset
	  // of the line map section in the cubin
	  line_map = cubin_ptr + shdr.sh_offset;

	  // found the line map, so we are done with the linear scan of sections

	  parseLineSection(ehdr, scn, &shdr, lih);
	  break;
	}
      }
    }
  }
}



//******************************************************************************
// interface functions
//******************************************************************************

bool
readCubinLineMap
(
 char *cubin_ptr,
 Elf *cubin_elf,
 LineInfoHandler *lih
)
{
  bool success = false;

  Elf_SectionVector *sections = elfGetSectionVector(cubin_elf);
  if (sections) {
    readLineMap(cubin_ptr, cubin_elf, sections, lih);
    delete sections;
  }

  return success;
}
