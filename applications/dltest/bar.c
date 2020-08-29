#include <stdio.h>

void init() __attribute__((constructor));

void init()
{
	printf("in bar constructor\n");
}
