all: main_fib.c fib.h  fib_omp.c
	gcc -g -O2 main_fib.c fib_omp.c -I. -o fib -fopenmp

check:
	~/pkgs/hpctoolkit-kernel/bin/hpcrun -e CYCLES -e INSTRUCTIONS -dd LINUX_PERF -t ./fib 44
	~/pkgs/hpctoolkit-kernel/bin/hpcstruct fib
	~/pkgs/hpctoolkit-kernel/bin/hpcprof -S fib.hpcstruct  hpctoolkit-fib-measurements/

clean:
	rm *.o
