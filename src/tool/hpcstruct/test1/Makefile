EXEC=struct-test

WRONG=$(EXEC)-wrong
RIGHT=$(EXEC)-right

all: $(WRONG) $(RIGHT) 

$(WRONG): struct-test.c 
	$(CC) -DMACRO=DECLARE_INLINE -g -o $@ $<
	hpcstruct $@
	hpcrun  -e cycles ./$@
	hpcprof -S $@.hpcstruct hpctoolkit-$@-measurements

$(RIGHT): struct-test.c 
	$(CC) -DMACRO=DECLARE -g -o $@ $<
	hpcstruct $@
	hpcrun  -e cycles ./$@
	hpcprof -S $@.hpcstruct hpctoolkit-$@-measurements

clean:
	/bin/rm -rf $(WRONG) $(WRONG).hpcstruct hpctoolkit-$(WRONG)-measurements hpctoolkit-$(WRONG)-database 
	/bin/rm -rf $(RIGHT) $(RIGHT).hpcstruct hpctoolkit-$(RIGHT)-measurements hpctoolkit-$(RIGHT)-database 
