#
#  Make.template
#
#  Copy this file to Makefile, fill in the paths and then run make.
#  Assume we're using new elfutils with libdw and not libdwarf.
#

CXX = g++
CXXFLAGS = -g -O -std=c++11

DYNINST = /home/jokeren/Install/dyninst
BOOST = /home/jokeren/Install/spack/linux-ubuntu18.04-x86_64/gcc-7.3.0/boost-1.65.1-yedailx37eqncxmqibk66elhc7pnmz3d
ELFUTILS = /home/jokeren/Install/spack/linux-ubuntu18.04-x86_64/gcc-7.3.0/elfutils-0.170-5vs7ecnlnxnobkfyx2m4fb6tnhlsrs2g
BZIP = /home/jokeren/Install/spack/linux-ubuntu18.04-x86_64/gcc-7.3.0/bzip2-1.0.6-jb5oxcpfhd4orv7dua5vukxtr6x45lit
LZMA = /home/jokeren/Install/spack/linux-ubuntu18.04-x86_64/gcc-7.3.0/lzma-4.32.7-vm3tyb6brnwygs3g27csif2hfbtgf6lb
ZLIB = /home/jokeren/Install/zlib

#------------------------------------------------------------

#  Hopefully, don't need to change anything below here.

DEFS = -DDYNINST_USE_CUDA

INCL =  \
    -I$(DYNINST)/include  \
    -I$(BOOST)/include  \
    -I$(ELFUTILS)/include  \
    -I$(ELFUTILS)/include/elfutils

LIBS =  \
    -L$(DYNINST)/lib  \
    -lparseAPI  -linstructionAPI  -lsymtabAPI  -ldynDwarf  -ldynElf  -lcommon  \
    -L$(BOOST)/lib  \
    -lboost_atomic  -lboost_chrono  -lboost_date_time  -lboost_filesystem  \
    -lboost_system  -lboost_thread  -lboost_graph\
    -L$(ELFUTILS)/lib  -lelf  -ldw  \
    -ldl

RPATH =  \
    -Wl,-rpath,$(DYNINST)/lib  \
    -Wl,-rpath,$(BOOST)/lib  \
    -Wl,-rpath,$(ELFUTILS)/lib  \
    -Wl,-rpath,$(BZIP)/lib  \
    -Wl,-rpath,$(LZMA)/lib  \
    -Wl,-rpath,$(ZLIB)/lib


PROG = cuda-parse

OBJS = cuda-parse.o  ElfHelper.o  Fatbin.o  InputFile.o  RelocateCubin.o  \
			 CudaCFGFactory.o  CudaCodeSource.o  CudaFunction.o  CudaBlock.o \
			 ReadCubinLineMap.o  LineMapping.o  GraphReader.o  CFGParser.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CXX)  -o $@  $(OBJS)  $(LIBS)  $(RPATH)

%.o: %.cpp
	$(CXX)  -c -o $@  $(CXXFLAGS)  $(DEFS)  $(INCL)  $<

clean:
	rm -f  *.o

vclean: clean
	rm -f  $(PROG)

