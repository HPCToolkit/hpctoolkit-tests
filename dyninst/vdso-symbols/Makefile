TARGET=symbols

ifndef EXT
ext_error:
	@echo "usage: make EXT=<hpctoolkit-externals install dir> ..."
endif

BOOST_ROOT=$(EXT)/boost
SYMTAB_ROOT=$(EXT)/symtabAPI

all: $(TARGET) test

$(TARGET):
	make -f Makefile.dyninst BOOST_ROOT=$(BOOST_ROOT) SYMTAB_ROOT=$(SYMTAB_ROOT) TARGET=$(TARGET)


test: $(TARGET)
	@for f in load-modules/*; do echo ./$(TARGET) $$f; ./$(TARGET) $$f; echo; done

clean:
	/bin/rm -f $(TARGET)


