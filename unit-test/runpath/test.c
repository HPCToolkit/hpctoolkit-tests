#include <dlfcn.h>
#include <stdio.h>

typedef int (*fnctype)();
fnctype ptr = NULL;
int main() {
    void* handle = dlopen("libfoo.so", RTLD_NOW);
    if (handle == NULL) {
        printf("Cannot find libfoo.so\n");
    } else {
        printf("Find libfoo.so\n");
	ptr = (fnctype)dlsym(handle, "foo");
	if (ptr == NULL) {
	    printf("Cannot find function foo\n");
	} else {
	    printf("Find function foo\n");
	}
    }
}
