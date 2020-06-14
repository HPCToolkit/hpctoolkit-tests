# CC=/usr/bin/gcc

all: run 

fib: fib.c 
	hpclink gcc -o $@  -static fib.c 

fib-pg: fib.c 
	hpclink gcc -o $@  -pg -static fib.c 

clean:
	/bin/rm -rf gmon.out fib fib-pg hpctoolkit-* *.hpcstruct

dbclean:
	/bin/rm -rf hpctoolkit-*

%.hpcstruct:
	hpcstruct %

run: fib fib-pg
	-hpcrun ./fib
	-hpcrun ./fib-pg
