INC=-I/usr/local/cuda-11.1/samples/common/inc
LIBS=-L/usr/local/cuda-11.1/lib64 -Wl,-rpath=/usr/local/cuda-11.1/lib64
OBJS=main.o vecadd.o

main: $(OBJS)
	mpicxx -o main $(OBJS) $(LIBS) -lcuda -lcudart

main.o: main.cpp
	mpicxx -c $<

vecadd.o: vecadd.cu
	nvcc -c $(INC) $<

run:
	$(LAUNCH) hpcrun ./main

clean:
	/bin/rm -rf *.o main hpctoolkit-*


