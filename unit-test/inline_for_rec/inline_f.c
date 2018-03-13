#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define ITER 1<<7

__attribute__((always_inline))
int f(int stop)
{
  int i, sum = stop;
  struct timespec req, rem;

  req.tv_sec  = 0;
  req.tv_nsec = 100000000;

#include "par_forloop.h"
  for (i=0; i<ITER; i++) {
    //sum += nanosleep(&req, &rem);
    if (stop > 0)
      sum += f(stop-1);
  }
  //printf(".");
  return sum;
}
