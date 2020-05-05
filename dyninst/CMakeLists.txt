cmake_minimum_required(VERSION 2.8)
project(toolkit_tests CXX)


find_package(Boost)
find_package(LibElf)
find_package(LibDwarf)
find_package(Dyninst 9.3.1 REQUIRED)
find_package(ZLIB)
find_package(LibIberty)

include_directories(${Boost_INCLUDE_DIR}
  ${LIBELF_INCLUDE_DIR}
  ${LIBDWARF_INCLUDE_DIR}
  ${DYNINST_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
  ${BINUTILS_INCLUDE_DIR})

add_executable(find-gaps find-gaps.cpp)
set_target_properties(find-gaps PROPERTIES
  COMPILE_FLAGS --std=c++11)
target_link_libraries(find-gaps parseAPI symtabAPI)


add_executable(cilk-parse cilk-parse.cpp)
set_target_properties(cilk-parse PROPERTIES
  COMPILE_FLAGS "-fcilkplus --std=c++11")
target_link_libraries(cilk-parse parseAPI cilkrts symtabAPI)
