#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dereference.h"

#define REPETITIONS 10
#define N 1000000000L


int
main(int argc, char **argv)
{
	long long result;
	for (int i = 0; i < REPETITIONS; i++) {
		int *data = (int *) malloc(N * sizeof(int));
		for (long j = 0; j < N; j++) {
			result += dereference(&data[j]);
		}
		free(data);
	}
	printf(" result = %lld\n", result);
}
