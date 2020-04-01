#
#  Makefile for catch-throw stress test.
#

CC = gcc
CCC = g++
CFLAGS = -g

PROGS = catchthrow

all: ${PROGS}

catchthrow: catchthrow.cc
	$(CCC) $(CFLAGS) -o $@ catchthrow.cc

test: ${PROGS}
	hpcrun -V
	export HPCRUN_ABORT_TIMEOUT=30; hpcrun -t -e REALTIME@250 catchthrow

clobber:
	/bin/rm -rf hpctoolkit* hpcstruct* core.*

clean: clobber
	rm -f $(PROGS)

