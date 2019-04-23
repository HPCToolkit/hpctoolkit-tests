#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NTHREADS 8
#define NBOUND 40
#define ARG 33
#define NITER 10000000

#define DEEP_WORK 0

uint64_t result[NBOUND]; 

#if DEEP_WORK
uint64_t burn(int n) 
{
  if (n < 2) return n;
  else return burn(n-1) + burn(n-2);
}
#else
uint64_t burn(int n) 
{
  int i;
  for (i = 0; i < NITER; i+=2) i--;
  return i;
}
#endif

void testparallel()
{
  int i;
#pragma omp parallel num_threads(NTHREADS) private(i)
  {
#pragma omp for
    for(i = 0; i < NBOUND; i++)  result[i] += burn(ARG);
#pragma omp for schedule(static)
    for(i = 0; i < NBOUND; i++)  result[i] += burn(ARG);
#pragma omp for schedule(dynamic)
    for(i = 0; i < NBOUND; i++)  result[i] += burn(ARG);
#pragma omp for schedule(guided)
    for(i = 0; i < NBOUND; i++)  result[i] += burn(ARG);
  }
}


void teststatic()
{
  int i;
#pragma omp parallel for schedule(static) private(i) num_threads(2)
  for(i = 0; i < NBOUND; i++) result[i] += burn(ARG);
}


void testdynamic()
{
  int i;
#pragma omp parallel for schedule(dynamic) private(i)
  for(i = 0; i < NBOUND; i++) result[i] += burn(ARG);
}


void testguided()
{
  int i;
#pragma omp parallel for schedule(guided) private(i)
  for(i = 0; i < NBOUND; i++) result[i] += burn(ARG);
}


int main(int argc, char **argv)
{
  testparallel();
  teststatic();
  testdynamic();
  testguided();
  int i;
  for(i=0;i< NBOUND;i++) printf("%" PRIu64 " ", result[i]);
  printf("\n");
  return 0;
}
