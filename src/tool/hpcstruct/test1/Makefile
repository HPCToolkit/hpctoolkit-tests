
CC = gcc
CXX = g++ -std=c++11

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
	$(CC) -c -g $<

lower_pic.o: lower.c
	$(CC) -c -g -fPIC $< -o $@

upper.o: upper.cpp
	$(CXX) -c -g $<

upper_pic.o: upper.cpp
	$(CXX) -c -g -fPIC $< -o $@

mixed: mixed.cpp lower.o upper.o
	$(CXX) -g $^ -o $@

libmix.so: lower_pic.o upper_pic.o
	$(CXX) -g -shared $^ -o $@

sort: sort.cpp
	$(CXX) -g -O2 $< -o $@

clean:
	/bin/rm -rf $(WRONG) $(WRONG).hpcstruct hpctoolkit-$(WRONG)-measurements hpctoolkit-$(WRONG)-database 
	/bin/rm -rf $(RIGHT) $(RIGHT).hpcstruct hpctoolkit-$(RIGHT)-measurements hpctoolkit-$(RIGHT)-database 
	rm -f $(PROGS) *.o *.so

