CC=gcc
CFLAGS=-O0 -fopenmp
LFLAGS=-fopenmp

all: omp_mm


omp_mm: omp_mm.o 
	$(CC) -g omp_mm.o -o omp_mm $(LFLAGS)

%.o: %.c
	$(CC) -c -g $(CFLAGS) -o $@ $<

clean:
	rm -f *.o inline

