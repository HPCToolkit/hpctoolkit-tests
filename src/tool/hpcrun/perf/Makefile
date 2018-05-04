all: run

main: main.c
	gcc -o main main.c

run: main
	hpcstruct main
	hpcrun -e cycles -e PERF_COUNT_HW_CACHE_L1D:READ -e PERF_COUNT_HW_CACHE_L1D:WRITE ./main
	hpcprof -S main.hpcstruct hpctoolkit-main-measurements
	hpcviewer hpctoolkit-main-database

clean:
	/bin/rm -rf main hpctoolkit-* *.hpcstruct
