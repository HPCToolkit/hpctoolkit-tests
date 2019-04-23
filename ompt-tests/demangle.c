#include <omp.h>
#include <stdio.h>

#define N 40
#define T 4 

void
loop(){
	int j;
	for(j=0;j<1000000000;j+=2) j--;
}

void
a()
{
	#pragma omp parallel num_threads(T)
	{
		loop();
		printf("Outer region: ID = %d\n", omp_get_thread_num());
	}
}

int
main()
{
  a();
  return 0;
}
