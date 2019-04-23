#include <omp.h>
#include <stdio.h>

long
fib(int n) {
  if (n < 2) return n;
  else return fib(n-1) + fib(n-2);
}

void
loop(){
	int j;
	for(j=0;j<1000000000;j+=2) j--;
}

void
loop2(){
	int j;
	for(j=0;j<100000000;j+=2) j--;
}

#define N 40
#define T 4 


void
a()
{
	long niz[7];
	#pragma omp parallel num_threads(T)
	{
		if(omp_get_thread_num() != 0) {
			loop2();
		}
	}
}

void
b(int slb)
{
	//a();
	int i=0;
	omp_lock_t lock;
	omp_init_lock(&lock);
	#pragma omp parallel num_threads(N)
	{
		int j;
		for(j=0;j<1000000000;j+=2) j--;
		//a();
		omp_set_lock(&lock);
		i++;
		omp_unset_lock(&lock);
	}
	omp_destroy_lock(&lock);
	printf("i=%d\n", i);
}

int
main()
{
  //omp_set_nested(1);
  a();
  return 0;
}
