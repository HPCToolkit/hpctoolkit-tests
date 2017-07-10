TARGET=nm-dso

OBJS = \
	main.o 


ifndef EXT_INSTALL
USAGE=1
endif

ifndef HPCT_SRCROOT
USAGE=1
endif

ifndef HPCT_BUILD
USAGE=1
endif

ifndef HPCT_INSTALL
USAGE=1
endif

ifdef USAGE
usage:
	@echo "usage: make EXT_INSTALL=<hpctoolkit-externals install dir> \\"
	@echo "            HPCT_SRCROOT=<hpctoolkit source dir> \\"
	@echo "            HPCT_INSTALL=<hpctoolkit install dir>"
	@echo "            HPCT_BUILD=<hpctoolkit build dir>"
endif

EU_ROOT=$(EXT_INSTALL)/libelf
ST_ROOT=$(EXT_INSTALL)/symtabAPI
DW_ROOT=$(EXT_INSTALL)/libdwarf

all: $(TARGET) test

BU_INCLUDE=-I$(HPCT_SRCROOT)/src/lib/binutils
BU_LIB=$(HPCT_BUILD)/src/lib/binutils/libHPCbinutils.la
PL_LIB=$(HPCT_INSTALL)/lib/hpctoolkit/libhpcrun.a 

ST_LIB=-L$(ST_ROOT)/lib -Wl,-rpath,$(ST_ROOT)/lib -lparseAPI -linstructionAPI -lsymtabAPI  
DW_LIB=-L$(DW_ROOT)/lib -Wl,-rpath,$(DW_ROOT)/lib -ldwarf 
EU_LIB=-L$(EU_ROOT)/lib -Wl,-rpath,$(EU_ROOT)/lib -lelf 


INCLUDES= \
	$(BU_INCLUDE)

LIBS= \
	$(BU_LIB) \
	$(PL_LIB) \
	$(ST_LIB) \
	$(DW_LIB) \
	$(EU_LIB) \
	-ldl

$(TARGET): main.cpp $(OBJS) 
	libtool --mode=link g++ -g $(INCLUDES) -o $@ $(OBJS) $(LIBS)

%.o: %.cpp
	g++ -c -g $(INCLUDES) -o $@ $<


test: $(TARGET)
	@for f in load-modules/*; do echo ./$(TARGET) $$f; ./$(TARGET) $$f; echo; done

clean:
	/bin/rm -f $(TARGET) *.o


