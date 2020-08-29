all: main foo.so bar.so run

main: main.c
	gcc -o main main.c -ldl

foo.so: foo.c
	gcc -fPIC -shared -o $@ $< -ldl

bar.so: bar.c
	gcc -fPIC -shared -o $@ $<

run:
	hpcrun ./main

clean:
	/bin/rm -rf hpctoolkit-main* foo.so main bar.so
