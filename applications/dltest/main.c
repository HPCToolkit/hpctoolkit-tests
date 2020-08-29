#include <stdio.h>
#include <dlfcn.h>
int main(int argc, char **argv)
{
	void * handle = dlopen("./foo.so", RTLD_LAZY);
}
