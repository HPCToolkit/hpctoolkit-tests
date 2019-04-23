#include <omp.h>
#include <stdio.h>

#define N 40
#define T 4

#define ITERATIONS 100000000

long
fib(int n) {
  if (n < 2) return n;
  else return fib(n-1) + fib(n-2);
}

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
a()
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
  omp_set_nested(1);
  a();
  return 0;
}
