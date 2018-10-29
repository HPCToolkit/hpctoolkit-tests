COMPILE_FIB=gcc -g -fopenmp -o fib fib.c

fib: fib.c
	$(COMPILE_FIB)

fib-static:
	hpclink $(COMPILE_FIB)

clean:
	/bin/rm -rf fib hpctoolkit-* *.hpcstruct
