#include <stdio.h>
#include <omp.h>
#include <stdio.h>

#define NTHREADS 4 
#define N 100000000

int
loop
(
  void
)
{
  volatile int j;
  for(j = 0; j < N; j += 2) j--;
  return j;
}


int
p1()
{
  int sum;
#pragma omp parallel num_threads(NTHREADS) reduction(+:sum)
  {
    int val = loop();
    sum += val;
  }
  return sum;
}

int
main
(
  int argc,
  char **argv
)
{
  printf("sum = %d\n", p1());
  return 0;
}
