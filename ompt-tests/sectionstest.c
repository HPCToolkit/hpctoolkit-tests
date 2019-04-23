#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NTHREADS 8
#define ITERATIONS 10000000

void
loop0()
{
  int j;
  for(j=0;j<ITERATIONS;j+=2) j--;
}


void
loop1()
{
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


void testsections()
{
  int i;
#pragma omp parallel num_threads(NTHREADS) private(i)
  {
    loop0();
#pragma omp sections
    {
#pragma omp section
      loop1();
#pragma omp section
      loop2();
#pragma omp section
      loop3();
    }
  }
}


void testparallelsections()
{
  int i;
#pragma omp parallel sections num_threads(NTHREADS) private(i)
  {
#pragma omp section
      loop0();
#pragma omp section
      loop1();
#pragma omp section
      loop2();
  }
}


int main(int argc, char **argv)
{
  testsections();
  testparallelsections();
  return 0;
}
