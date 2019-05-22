#include <omp.h>
#include <stdio.h>

#define ITERATIONS 100000000

void
loop0(){
  int j;
  for(j=0;j<ITERATIONS;j+=2) j--;
}

void
loop1(){
  int j;
  for(j=0;j<ITERATIONS;j+=2) j--;
}

void
loop2(){
  int j;
  for(j=0;j<ITERATIONS;j+=2) j--;
}

void
loop3(){
  int j;
  for(j=0;j<ITERATIONS;j+=2) j--;
}

void
p1()
{
#pragma omp parallel num_threads(2)
  {
    loop0();
#pragma omp parallel num_threads(2)
    {
      loop1();
#pragma omp parallel num_threads(3)
      {
	loop2();
#pragma omp parallel num_threads(3)
	{
	  loop3();
	}
      }
    }
  }
}

int
main()
{
  p1();
  return 0;
}
