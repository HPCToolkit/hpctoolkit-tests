all: run

main: main.c
	gcc -o main main.c

cycles: main
	hpcrun -e cycles -o cycles ./main

cycles_writes: main
	hpcrun -e cycles -e PERF_COUNT_HW_CACHE_L1D:WRITE -o cycles_writes ./main

cycles_reads: main
	hpcrun -e cycles -e PERF_COUNT_HW_CACHE_L1D:READ -o cycles_reads ./main

cycles_reads_writes:
	hpcrun -e cycles -e PERF_COUNT_HW_CACHE_L1D:READ -e PERF_COUNT_HW_CACHE_L1D:WRITE -o cycles_reads_writes ./main

run: main.hpcstruct cycles cycles_writes cycles_reads cycles_reads_writes
	hpcprof -S main.hpcstruct -o cycles-database cycles
	hpcprof -S main.hpcstruct -o cycles_reads-database cycles_reads
	hpcprof -S main.hpcstruct -o cycles_writes-database cycles_writes
	hpcprof -S main.hpcstruct -o cycles_reads_writes-database cycles_reads_writes

main.hpcstruct: main
	hpcstruct main

clean:
	/bin/rm -rf main main.hpcstruct cycles cycles_writes cycles_reads cycles_reads_writes *database*

