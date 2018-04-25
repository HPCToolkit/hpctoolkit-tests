#include <stdio.h>
#include <omp.h>

#define N 41

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
#pragma omp parallel 
{
	int tid = omp_get_thread_num();
	printf("thread %d fib(%d) = %ld\n", tid, (int) N, fib(max(1, N-tid/4)));
}

	return 0;
}
