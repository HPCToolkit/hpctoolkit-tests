#include <omp.h>
#include <stdio.h>

#define NTHREADS 4 
#define N 100000000

void
loop
(
  void
)
{
  int j;
  for(j = 0; j < N; j += 2) j--;
}


void
p1()
{
#pragma omp parallel num_threads(NTHREADS)
  {
    loop();
  }
}

int
main
(
  int argc,
  char **argv
)
{
  p1();
  return 0;
}
