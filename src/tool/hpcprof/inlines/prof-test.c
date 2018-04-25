#define f fffff
#define g ggggg
#define e eeeee

#define N 100000000


volatile int ctr = 0;


__attribute__((always_inline))
inline
void e()
{
	int i;
	for(i = 0; i < N; i++) {
		ctr++;	
	}
}


__attribute__((always_inline))
inline
void f()
{
	int i;
	for(i = 0; i < N; i++) {
		ctr++;	
	}
        e();
}

__attribute__((always_inline))
inline
void g()
{
	int i;
#include "prof-test.h"
	for(i = 0; i < N; i++) {
		ctr++;	
	}
	f();
}

int main(int argc, char **argv)
{
#pragma omp parallel num_threads(2)
{
	g();
}
	return 0;
}
