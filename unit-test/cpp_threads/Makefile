all: main_fib.cxx  fib.h  fib_thread.cxx
	g++ -g -O2 main_fib.cxx fib_thread.cxx -I. -o fib  -lpthread

check:
	hpcrun -e CYCLES -e INSTRUCTIONS -dd LINUX_PERF -t ./fib 44
	hpcstruct fib
	hpcprof -S fib.hpcstruct  hpctoolkit-fib-measurements/

clean:
	rm *.o
