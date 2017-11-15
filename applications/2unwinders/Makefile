OFLAGS =-O2 -fopenmp
OBJS = 2unwinders-omp
CXX=icpc

all: $(OBJS)

% : %.cpp 
	$(CXX) $(OFLAGS) -o $@ $<

clean:
	/bin/rm -rf $(OBJS)
