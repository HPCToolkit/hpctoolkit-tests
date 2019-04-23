include config

# guess versions of gcc if none specified
ifndef GCC
GCC=`which gcc`
endif

ifndef GXX
GCC=`which g++`
endif

#tell the compiler to bind to the openmp library in the path specified by OLIB 
OMPT_LIBS=-L$(OLIB) -Wl,-rpath=$(OLIB) -lomp

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
