#include <stdio.h>

#define N 38

static long
fib(int n)
{
	if (n < 2) return n;
	return fib(n-1) + fib(n-2);
}

long
bar()
{
	return fib(N);
}
