H=/home/johnmc/pkgs-src/hpctoolkit-openmp/INSTALL-7.3.0/bin
OMPT_LIBPATH=/home/johnmc/pkgs-src/openmp/INSTALL/lib

LDFLAGS +=-L$(OMPT_LIBPATH) -Wl,-rpath=$(OMPT_LIBPATH) -lomp 

test: fopenmp
	$(H)/hpcrun -e REALTIME@1000 -t ./fopenmp 
	$(H)/hpcstruct ./fopenmp 
	$(H)/hpcprof -S fopenmp.hpcstruct hpctoolkit-fopenmp-measurements
	
	
fopenmp: fopenmp.f90 Makefile
	gfortran -g -o fopenmp -fopenmp fopenmp.f90 $(LDFLAGS)

clean:
	/bin/rm -rf fopenmp hpctoolkit-* *.hpcstruct
