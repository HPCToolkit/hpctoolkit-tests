TARGET=vdso-query
CC=gcc

OBJS = main.o 

ifndef HPCT_SRCROOT
USAGE=1
endif

ifndef HPCT_INSTALL
USAGE=1
endif

ifdef USAGE
usage:
	@echo "usage: make HPCT_SRCROOT=<hpctoolkit source dir> \\"
	@echo "            HPCT_INSTALL=<hpctoolkit install dir>"
endif

all: $(TARGET) 

PL_INCLUDE=-I$(HPCT_SRCROOT)/src/lib/prof-lean
PL_LIB=$(HPCT_INSTALL)/lib/hpctoolkit/libhpcrun.a

INCLUDES= \
	$(PL_INCLUDE)

LIBS= \
	$(PL_LIB)

$(TARGET): main.c $(OBJS) 
	libtool --mode=link --tag=CC $(CC) -g $(INCLUDES) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	gcc -c -g $(INCLUDES) -o $@ $<


clean:
	/bin/rm -f $(TARGET) *.o maps-* "[vdso]"-*


