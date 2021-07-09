#include "LineMapping.hpp"

#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <libdw.h>

#include <LineInformation.h>
#include <Function.h>
#include <Module.h>

#include "LineMapping.hpp"
#include "ReadCubinLineMap.hpp"

using namespace Dyninst;
using namespace SymtabAPI;

static std::vector<Line> line_mappings;

void CudaLineInfoDecoder::reportLine(uint64_t start_address, uint64_t end_address,
  uint16_t file_number, uint32_t line, uint32_t column, FileSystem *fs) {
	Line line_mapping(start_address, end_address, file_number, line, column);
	line_mappings.push_back(line_mapping);
}


bool LineMapping::read_lines(const std::string &relocated_cubin) {
  int fd = open(relocated_cubin.c_str(), O_RDONLY);
  if (fd == -1) {
    return false;
  }

  struct stat sb;
  fstat(fd, &sb);
  void *p = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, (off_t)0); 
  if (p == MAP_FAILED) {
    return false;
  }

  // create a memory-resident copy of the file that we can read and write 
  char *memPtr = (char *)malloc(sb.st_size); 
  if (memPtr == 0) {
    return false;
  }

  memcpy(memPtr, p, sb.st_size); 

  munmap(p, sb.st_size); 

  Elf *elf = elf_memory(memPtr, sb.st_size);

  Dwarf *dbg = dwarf_begin_elf(elf, DWARF_C_READ, 0);

  if (dbg == NULL) {
    readCubinLineMap(memPtr, elf, &this->_cuda_line_info_decoder);
  }

  elf_end(elf);
  free(memPtr);
  close(fd);
  return dbg == NULL;
}


bool LineMapping::insert_lines(Symtab *symtab) {
  for (auto &line : line_mappings) {
    SymtabAPI::Function *sym_func = NULL;
    symtab->getContainingFunction(line.start_address, sym_func);
    Module * mod = NULL;
    if (sym_func != NULL) {
      mod = sym_func->getModule();
    } else {
      return false;
    }

    if (mod != NULL) {
      LineInformation *line_information = mod->getLineInformation();
      if (line_information == NULL) {
        line_information = new LineInformation();
        if (line_information->addLine(line.file, line.line, line.column,
                                      line.start_address, line.end_address)) {
          mod->setLineInfo(line_information);
        } else {
          return false;
        }
      } else {
        if (!line_information->addLine(line.file, line.line, line.column,
                                       line.start_address, line.end_address)) {
          return false;
        }
      }
    } else {
      return false;
    }
  }
  return true;
}

