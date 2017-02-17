HPC=../../../../hpctoolkit
LEAN_INCL=-I$(HPC)/src/lib/prof-lean
LEAN_LIBS=$(HPC)/INSTALL/lib/hpctoolkit/libhpcrun.a
CFLAGS = -O3 -g -Wall $(LEAN_INCL) -std=c99
LOCK_OBJS = lock-test.o
RWLOCK_OBJS = rwlock-test.o

all: lock-test rwlock-test

lock-test: $(LOCK_OBJS)
	$(CC) $(CFLAGS) -o $@ $(LOCK_OBJS) $(LEAN_LIBS) -pthread -lm

lock-test.o: lock-test.c
	$(CC) $(CFLAGS) -c -o $@ $<

rwlock-test: $(RWLOCK_OBJS)
	$(CC) $(CFLAGS) -o $@ $(RWLOCK_OBJS) $(LEAN_LIBS) -pthread -lm

rwlock-test.o: rwlock-test.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o lock-test rwlock-test
