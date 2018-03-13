

#define ITER_OUT 1<<14
#define ITER_IN  1<<16


__attribute__((always_inline))
long
LINE_STATEMENT(int j, int stop)
{
   return  stop +  j;
}

__attribute__((always_inline))
long
FOR_INNERLOOP(int stop) 
{
  int  i, j;
  long sum;

#include "par_forloop.h"
  for ( i=0;  i<ITER_OUT;  i++) {
    for (j=ITER_IN; j>0; j--) { 
	 sum +=   LINE_STATEMENT(j, stop);  
    }
  }
  return sum;
}



__attribute__((always_inline))
long
FOR_OUTERLOOP(int stop) 
{
  return FOR_INNERLOOP(stop); 
}


__attribute__((always_inline))
long 
g(int stop)
{
  int sum = stop;

  sum += FOR_OUTERLOOP(sum - 1);
   
  return sum;
}

__attribute__((always_inline))
long 
f(int stop)
{
  long sum = stop + g(stop - 1);
  return sum;
}
