#ifndef __LineInfoDecoderDump_hpp__
#define __LineInfoDecoderDump_hpp__

//******************************************************************************
// local include files
//******************************************************************************

#include "LineInfoDecoder.hpp"

#define DUMP_DECODED_LINE 1

//******************************************************************************
// macros
//******************************************************************************

#if DUMP_DECODED_LINE 
#define DUMP_DECODED_OUTPUT(...) std::cout << __VA_ARGS__
#else
#define DUMP_DECODED_OUTPUT(...)
#endif



//******************************************************************************
// type declarations
//******************************************************************************

class LineInfoDecoderDump : public LineInfoDecoder {
public:

  virtual void reportLine
  (
    uint64_t start_address,
    uint64_t end_address,
    const char *dir_name,
    const char *file_name,
    uint32_t line, 
    uint32_t column 
  ) {
    DUMP_DECODED_OUTPUT(file_name << "\t\t" << line << "\t\t[");
    dumpAddr(start_address);
    DUMP_DECODED_OUTPUT(", ");
    dumpAddr(end_address);
    DUMP_DECODED_OUTPUT(")\n");
  };


  void dumpAddr(uint64_t addr) {
    if (addr == 0) 
      DUMP_DECODED_OUTPUT("0x"); // the line below drops the 0x for 0
    DUMP_DECODED_OUTPUT(std::hex << (void *) addr << std::dec);
  };

  virtual void processMatrixRow
  (
    LineInfo *li,
    FileSystem *fs
  ) {
    if (first) {
      DUMP_DECODED_OUTPUT("CU: " << fs->getDirName(li->file) <<
                          "/" << fs->getFileName(li->file) << "\n");
      DUMP_DECODED_OUTPUT("File name                            Line number"
                      "    Starting address    View\n");
    }
    LineInfoDecoder::processMatrixRow(li, fs);
  };
};

#endif
