all: main_fib.cxx  fib.h  fib_thread.cxx
	g++ -g -O0 main_fib.cxx fib_thread.cxx -I. -o fib  -lpthread

check:
	time ./fib 8 3
	time hpcrun -dd LINUX_PERF -e cycles -t ./fib 8 3
	time hpcstruct fib
	time mpirun -np 4 hpcprof-mpi -S fib.hpcstruct  hpctoolkit-fib-measurements/

clean:
	rm *.o
