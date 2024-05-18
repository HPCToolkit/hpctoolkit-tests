#ifndef _LINE_MAPPING_H_
#define _LINE_MAPPING_H_

#include <map>
#include <string>
#include <vector>

#include <Symtab.h>
#include <dyntypes.h>

#include "LineInfoDecoder.hpp"

struct Line {
	uint64_t start_address;
	uint64_t end_address;
	uint16_t file;
	uint32_t line;
	uint32_t column;

	Line(uint64_t start_address, uint64_t end_address, 
		uint16_t file, uint32_t line, uint32_t column) : 
		start_address(start_address), end_address(end_address),
		file(file), line(line), column(column) {}
};


class CudaLineInfoDecoder : public LineInfoDecoder {
	public:
   CudaLineInfoDecoder() {}  

	 virtual void reportLine(uint64_t start_address, uint64_t end_address,
	 	 uint16_t line_number, uint32_t line, uint32_t column, FileSystem *fs);

   virtual void processMatrixRow(LineInfo *li, FileSystem *fs) {
     LineInfoDecoder::processMatrixRow(li, fs);
   }
};


class LineMapping {
 public:
  LineMapping() {}

  bool read_lines(const std::string &cubin_file);
  bool insert_lines(Dyninst::SymtabAPI::Symtab *symtab);

 private:
  CudaLineInfoDecoder _cuda_line_info_decoder;
};

#endif
