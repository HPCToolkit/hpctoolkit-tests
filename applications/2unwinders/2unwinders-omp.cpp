#define N 42
#include <stdio.h>

long a(int n)
{
	if (n < 2) return n;
	return a(n-1)+a(n-2);
}

long b(int n)
{
	if (n < 2) return n;
	return b(n-1)+b(n-2);
}


long c(int n)
{
	if (n < 2) return n;
	return c(n-1)+c(n-2);
}

int main(int argc, char **argv)
{
	printf("%ld\n", a(N));
#pragma omp parallel 
	{
		printf("%ld\n", b(N));
	}
	printf("%ld\n", c(N));
	return 0;
}
