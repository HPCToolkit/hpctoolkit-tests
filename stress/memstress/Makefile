#
#  Makefile for memory stress test.
#

CC = gcc
CFLAGS = -g -O -Wall

PROGS = memstress

memstress: memstress.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

clean:
	rm -f $(PROGS)

