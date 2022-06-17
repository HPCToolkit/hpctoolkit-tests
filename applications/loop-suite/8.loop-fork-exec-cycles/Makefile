COMPILER=gcc
COMPILER_ARGS=-g -O2
MEASUREMENT_ARGS=-e cycles


all: child loop.d

child:
	make -f Makefile.child

clean::
	make -f Makefile.child clean

TARGET=loop
include ../include/Makefile
