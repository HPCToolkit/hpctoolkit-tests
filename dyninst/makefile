TARGET=openmp-parse

BOOST_ROOT=/path/to/boost
DYNINST_ROOT=/path/to/dyninst

PMODEL_OPT=-fopenmp

MAKECMD=\
	make -f Makefile \
		TARGET=$(TARGET) \
		BOOST_ROOT=$(BOOST_ROOT) \
		DYNINST_ROOT=$(DYNINST_ROOT) \
		PMODEL_OPT=$(PMODEL_OPT)

build:
	$(MAKECMD) build

clean:
	$(MAKECMD) clean

