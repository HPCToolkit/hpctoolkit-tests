#include <stdlib.h>

#define N 100000000
int main(int argc, char **argv)
{
	int *x = malloc(N * sizeof(int));
	for (int i=0; i<N; i+=4096) {
		x[i] = 10;
	}
}
