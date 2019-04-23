COMPILE_FIB=gcc -g -fopenmp -o fib fib.c

fib: fib.c
	$(COMPILE_FIB)

fib-static:
	hpclink gcc -g -static -fopenmp -o fib fib.c

clean:
	/bin/rm -rf fib hpctoolkit-* *.hpcstruct
