OFLAGS =-O2
OBJS = 2unwinders-omp

all: $(OBJS)

% : %.cpp 
	g++ $(OFLAGS) -o $@ $<

clean:
	/bin/rm -rf $(OBJS)
