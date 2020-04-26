#include <stdio.h>

#include "libfib.h"

#define N 38

#define max(x,y) (x < y) ? y : x

long
fib(int n)
{
	if (n < 2) return n;
	return libfib(n-1, fib) + libfib(n-2, fib);
}

int
main(int argc, char **argv)
{
	printf("fib(%d) = %ld\n", (int) N, fib(N));
	return 0;
}
