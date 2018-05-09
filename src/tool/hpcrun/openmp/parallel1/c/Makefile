H=/home/johnmc/pkgs-src/hpctoolkit-openmp/INSTALL-7.3.0/bin
OMPT_LIBPATH=/home/johnmc/pkgs-src/openmp/INSTALL/lib

LDFLAGS +=-L$(OMPT_LIBPATH) -Wl,-rpath=$(OMPT_LIBPATH) -lomp 

test: copenmp
	$(H)/hpcrun -e REALTIME@1000 -t ./copenmp 
	$(H)/hpcstruct ./copenmp 
	$(H)/hpcprof -S copenmp.hpcstruct hpctoolkit-copenmp-measurements
	
	
copenmp: copenmp.c Makefile
	gcc -g -o copenmp -fopenmp copenmp.c $(LDFLAGS)

clean:
	/bin/rm -rf copenmp hpctoolkit-* *.hpcstruct
