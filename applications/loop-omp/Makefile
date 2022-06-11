# CC=gcc
CC=CC -dynamic 
CC=icpx -g -fopenmp

loop: loop.c
	$(CC) -o $@ $<

clean:
	rm -rf loop hpctoolkit*
