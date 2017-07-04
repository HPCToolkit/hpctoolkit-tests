TARGET=nm-dso

OBJS = \
	main.o 


ifndef EXT_INSTALL
USAGE=1
endif

ifndef HPCT_SRCROOT
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
endif

EU_ROOT=$(EXT_INSTALL)/libelf

all: $(TARGET) test

PL_INCLUDE=-I$(HPCT_SRCROOT)/src/lib/prof-lean
PL_LIB=$(HPCT_INSTALL)/lib/hpctoolkit/libhpcrun.a

EU_INCLUDE=-I$(EU_ROOT)
EU_LIB=-L$(EU_ROOT)/lib -Wl,-rpath,$(EU_ROOT)/lib -lelf 

# EU_ROOT=/home/johnmc/pkgs-src/elfutils/libelf
# EU_INCLUDE=-I$(EU_ROOT)
# EU_LIB=-L$(EU_ROOT) -Wl,-rpath,$(EU_ROOT) -lelf 


INCLUDES= \
	$(PL_INCLUDE) \
	$(EU_INCLUDE) 

LIBS= \
	$(PL_LIB) \
	$(EU_LIB) \
	-ldl

$(TARGET): main.c $(OBJS) 
	libtool --mode=link gcc -g $(INCLUDES) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	gcc -c -g $(INCLUDES) -o $@ $<


test: $(TARGET)
	@for f in load-modules/*; do echo ./$(TARGET) $$f; ./$(TARGET) $$f; echo; done

clean:
	/bin/rm -f $(TARGET) *.o


