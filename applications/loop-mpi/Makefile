# CC=gcc
DFLAG=-dynamic
CC=CC
OFLAG=-g

all: loop-mpi loop-mpi-static 

loop-mpi: loop-mpi.c
	$(CC) $(OFLAG) $(DFLAG) -o $@ $<
	hpcstruct $@


loop-mpi-static: loop-mpi.c
	hpclink $(CC) $(OFLAG) -o $@ $<
	hpcstruct $@

clean:
	rm -f loop-mpi loop-mpi-static
