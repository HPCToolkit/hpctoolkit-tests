SERIAL=prof-test
OMP=$(SERIAL)-omp

all: $(SERIAL) $(OMP)

$(SERIAL): prof-test.c prof-test.h prof-test2.h
	$(CC) -g -o $@ $<
	hpcstruct $@
	hpcrun  -e cycles ./$@
	hpcprof -S $@.hpcstruct hpctoolkit-$@-measurements

$(OMP): prof-test.c prof-test.h prof-test2.h
	$(CC) -fopenmp -g -o $@ $<
	hpcstruct $@
	hpcrun  -e cycles ./$@
	hpcprof -S $@.hpcstruct hpctoolkit-$@-measurements
clean:
	/bin/rm -rf $(SERIAL) $(SERIAL).hpcstruct hpctoolkit-$(SERIAL)-measurements hpctoolkit-$(SERIAL)-database
	/bin/rm -rf $(OMP) $(OMP).hpcstruct hpctoolkit-$(OMP)-measurements hpctoolkit-$(OMP)-database
