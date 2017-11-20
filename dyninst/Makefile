ifndef BOOST_ROOT
ext_error1:
	@echo "usage: make BOOST_ROOT=</path/to/boost/install> ..."
endif

ifndef DYNINST_ROOT
ext_error2:
	@echo "usage: make DYNINST_ROOT=</path/to/dyninst/install> ..."
endif

ifndef TARGET
ext_error3:
	@echo "usage: make TARGET=<targetname> ..."
endif


ifndef PMODEL_OPT
ext_error3:
	@echo "usage: make PMODEL_OPT=<compiler opt for programming model> ..."
endif

build: $(TARGET) 

$(TARGET):
	make -f Makefile.dyninst BOOST_ROOT=$(BOOST_ROOT) DYNINST_ROOT=$(DYNINST_ROOT) TARGET=$(TARGET) PMODEL_OPT=$(PMODEL_OPT)


test: $(TARGET)
	@for f in load-modules/*; do echo time ./$(TARGET) $$f; time ./$(TARGET) $$f; echo; done

all: build test

clean:
	/bin/rm -f $(TARGET)


