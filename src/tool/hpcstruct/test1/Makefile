EXEC=struct-test

WRONG=$(EXEC)-wrong
RIGHT=$(EXEC)-right

PROGS = $(WRONG) $(RIGHT) mixed sort

all: $(PROGS)

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

lower.o: lower.c
	gcc -c -g $<

mixed: mixed.cpp lower.o
	g++ -g $^ -o $@

sort: sort.cpp
	g++ -g -O2 -std=c++11 $< -o $@

clean:
	/bin/rm -rf $(WRONG) $(WRONG).hpcstruct hpctoolkit-$(WRONG)-measurements hpctoolkit-$(WRONG)-database 
	/bin/rm -rf $(RIGHT) $(RIGHT).hpcstruct hpctoolkit-$(RIGHT)-measurements hpctoolkit-$(RIGHT)-database 
	rm -f $(PROGS) *.o

