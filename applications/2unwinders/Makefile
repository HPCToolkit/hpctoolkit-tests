OFLAGS =-O2 -fopenmp
OBJS = 2unwinders-omp
CXX=icpc

all: $(OBJS) dump

% : %.cpp 
	$(CXX) $(OFLAGS) -o $@ $<

dump: $(OBJS)
	objdump -D $(OBJS) > $@

clean:
	/bin/rm -rf $(OBJS) dump
