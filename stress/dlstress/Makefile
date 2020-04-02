#
#  Makefile for dlopen stress test.
#

CC = gcc
CFLAGS = -g -O -Wall

PROGS = dlstress libsum1.so libsum2.so

dlstress: libsum1.so libsum2.so dlstress.c
	$(CC) $(CFLAGS) -o $@ dlstress.c -ldl -lpthread

libsum1.so: sum.c
	$(CC) $(CFLAGS) -o $@ -shared -fPIC $<

libsum2.so: sum.c
	$(CC) $(CFLAGS) -o $@ -shared -fPIC -DLIBSUM_TWO $<

clean:
	rm -f $(PROGS)

