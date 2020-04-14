TARGET=linemap

BOOST_ROOT=/home/johnmc/pkgs/boost
DYNINST_ROOT=/home/dougm/pkgs-src/dyninst/INSTALL-dyninst
DYNINST_ROOT=/home/johnmc/pkgs-src/dyninst-master/dyninst-install
DYNINST_ROOT=/home/johnmc/pkgs-src/dyninst/INSTALL-dyninst

TBB_ROOT=-I/projects/spack/opt/spack/linux-rhel7-x86_64/gcc-7.3.0/intel-tbb-2019.2-zbzlnrvzthswcc7piwbwfm5wfpiz7dkd/include

PMODEL_OPT=-fopenmp

MAKECMD=\
	make -f Makefile \
		TARGET=$(TARGET) \
		BOOST_ROOT=$(BOOST_ROOT) \
		DYNINST_ROOT=$(DYNINST_ROOT) \
		TBB_ROOT=$(TBB_ROOT) \
		PMODEL_OPT=$(PMODEL_OPT)

build:
	$(MAKECMD) build

clean:
	$(MAKECMD) clean

