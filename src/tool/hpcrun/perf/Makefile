#D=-dd LINUX_PERF
C=-e cycles
L=-e L1-DCACHE-LOADS
S=-e L1-DCACHE-STORES

EXP=cycles load store cycles_store cycles_load cycles_load_store

all: run

main: main.cpp
	g++ -o main -g main.cpp

cycles: main
	hpcrun $(D) $(C) -o cycles ./main

store: main
	hpcrun $(D) $(S) -o store ./main

load: main
	hpcrun $(D) $(L) -o load ./main

cycles_store: main
	hpcrun $(D) $(C) $(S) -o cycles_store ./main

cycles_load: main
	hpcrun $(D) $(C) $(L) -o cycles_load ./main

cycles_load_store:
	hpcrun $(D) $(C) $(L) $(S) -o cycles_load_store ./main

run: main.hpcstruct  $(EXP)
	hpcprof -S main.hpcstruct -o cycles-database cycles
	hpcprof -S main.hpcstruct -o load-database load
	hpcprof -S main.hpcstruct -o store-database store
	hpcprof -S main.hpcstruct -o cycles_load-database cycles_load
	hpcprof -S main.hpcstruct -o cycles_store-database cycles_store
	hpcprof -S main.hpcstruct -o cycles_load_store-database cycles_load_store

main.hpcstruct: main
	hpcstruct main

clean:
	/bin/rm -rf main main.hpcstruct $(EXP) *database*
