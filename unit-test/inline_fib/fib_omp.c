int 
fib_omp(int n) {
  int i, j;
  if (n<2)
    return n;
  else {
    #pragma omp task shared(i)
    i=fib_omp(n-1);
    #pragma omp task shared(j)
    j=fib_omp(n-2);
#include "directive.h"
    return i+j;
  }
}
