# CC=/usr/bin/gcc

all: run 

fib-bar: fib.c bar.so
	gcc -o $@  fib.c -Wl,-rpath=`pwd` bar.so 

fib-pg-bar: fib.c bar-pg.so
	gcc -o $@  -pg fib.c -Wl,-rpath=`pwd` bar.so 

fib-pg-bar-pg: fib.c bar-pg.so
	gcc -o $@  -pg fib.c -Wl,-rpath=`pwd` bar-pg.so 

fib-bar-pg: fib.c bar-pg.so
	gcc -o $@  fib.c -Wl,-rpath=`pwd` bar-pg.so 


bar.so: bar.c
	gcc -shared -fPIC -o bar.so bar.c

bar-pg.so: bar.c
	gcc -shared -fPIC -pg -o bar-pg.so bar.c
	
clean:
	/bin/rm -rf gmon.out bar.so bar.o fib-* bar-* hpctoolkit-* *.hpcstruct

dbclean:
	/bin/rm -rf hpctoolkit-*

%.hpcstruct:
	hpcstruct %

run: fib-bar fib-pg-bar fib-pg-bar-pg fib-bar-pg
	-hpcrun ./fib-bar
	-hpcrun ./fib-pg-bar
	-hpcrun ./fib-pg-bar-pg
	-hpcrun ./fib-bar-pg
