COMPILER=gcc
COMPILER_ARGS=-g -O2
MEASUREMENT_ARGS=-t -e cycles


all: child loop.d

child:
	make -f Makefile.child

clean::
	make -f Makefile.child clean

LAUNCHER=mpirun  -np 2
TARGET=loop
include ../include/Makefile
