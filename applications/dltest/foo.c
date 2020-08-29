#include <stdio.h>
#include <dlfcn.h>

void init() __attribute__((constructor));

void init()
{
	void *handle = dlopen("./bar.so", RTLD_LAZY);
	printf("in foo constructor\n");
}
