PTH=/home/johnmc/spack/opt/spack/linux-rhel7-x86_64/gcc-4.8.5/gcc-7.3.0-2bfpv2u6zl5wux44mswawrkpzh2fillo/bin

ifeq ($(V),4)
PTH=/usr/bin
else
V=5+
PTH=/opt/apps/software/Core/GCCcore/6.4.0/bin
PTH=/home/johnmc/spack/opt/spack/linux-rhel7-x86_64/gcc-4.8.5/gcc-7.3.0-2bfpv2u6zl5wux44mswawrkpzh2fillo/bin

endif

GCC=$(PTH)/gcc
GPP=$(PTH)/g++

HPCT=/home/johnmc/pkgs-src/openmp-5.0/hpctoolkit-vladimir/hpctoolkit-install/bin

VIEWERS=/home/johnmc/pkgs/hpctoolkit/bin
VIEWERS=/projects/pkgs/hpctoolkit-viewers/bin

# llvm-openmp mirror
OLIB=/home/johnmc/pkgs-src/llvm-openmp-5/openmp-install/lib
OLIB=/home/johnmc/pkgs-src/openmp-5.0/openmp/openmp-install/lib

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
