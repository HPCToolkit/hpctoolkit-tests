all: test libfoo.so

test: test.c
	gcc -o test test.c -O0 -ldl -Wl,--enable-new-dtags -Wl,--rpath=`pwd` 

libfoo.so: libfoo.c
	gcc -o libfoo.so -fPIC -shared libfoo.c -O0

run:
	./test
	hpcrun ./test
	hpcrun --disable-rtld-audit ./test

clean:
	rm -rf test libfoo.so hpctoolkit-test-measurements*
