include config

# guess versions of gcc if none specified
ifndef GCC
GCC=`which gcc`
endif

#tell the compiler to bind to the openmp library in the path specified by OLIB 
ifdef OMP
#OMPT_LIBS=-I $(OMP)/include -L$(OMP)/lib -Wl,-rpath=$(OMP)/lib -lomp
OMPT_LIBS=-I$(OMP)/include -L$(OMP)/lib -lomp
endif

#build without echoing current directory with each recursive invocation of make
MAKE=make --no-print-directory

goals:
	$(MAKE) -f Makefile.run goals

one:
	$(MAKE) V=$(V) TEST=$@ G=$(G) any
three:
	$(MAKE) V=$(V) TEST=$@ G=$(G) any

looptest:
	$(MAKE) V=$(V) TEST=$@ G=$(G) any

fibtask:
	$(MAKE) V=$(V) TEST=$@ G=$(G) any


any:
	$(MAKE) -f Makefile.run V=$(V) TRACING=$(T) TEST=$(TEST) GCC=$(GCC) HPCT=$(HPCT) OMPT_LIBS="$(OMPT_LIBS)" VIEWERS=$(VIEWERS) $(G)

clean:
	/bin/rm -rf *.o *.hpcstruct hpctoolkit* thread* *.exe
