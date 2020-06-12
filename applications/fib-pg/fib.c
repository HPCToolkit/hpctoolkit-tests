#include <stdio.h>

#define N 38

#define max(x,y) (x < y) ? y : x

long
fib(int n)
{
	if (n < 2) return n;
	return fib(n-1) + fib(n-2);
}

int
main(int argc, char **argv)
{
{
	int tid = 1;
	printf("thread %d fib(%d) = %ld\n", tid, (int) N, fib(max(1, N-tid/4)));
}

	return 0;
}
