COMPILE=gcc -g 

TARGETS=fib libfib.so


# create an unreadable executable to test hpctoolkit
# with circumstances we see at LLNL with 
# export-controlled apps
fib: fib.c libfib.so
	$(COMPILE) -o fib fib.c -Wl,-rpath=. libfib.so
	chmod -r $@

# note: if libfib.so is not readable, ld.so fails
#       therefore, we don't have to consider the case of
#       unreadable shared libraries 
libfib.so: libfib.c
	$(COMPILE) -shared -fPIC -o libfib.so libfib.c
	

fib-static:
	hpclink gcc -g -static -fopenmp -o fib fib.c -lomp

clean:
	/bin/rm -rf $(TARGETS) hpctoolkit-* *.hpcstruct

dbclean:
	/bin/rm -rf hpctoolkit-*


run: fib 
	hpcrun -e REALTIME ./fib
	hpcprof hpctoolkit-fib-measurements
	hpcviewer hpctoolkit-fib-database
