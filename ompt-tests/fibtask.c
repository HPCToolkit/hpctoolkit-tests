#include <stdio.h>
#include <omp.h>

#define THRESH 25

int fib(int n)
{
  int i, j;
  if (n<2)
    return n;
  else
    {
       #pragma omp task shared(i) firstprivate(n) if (n - 1 > THRESH)
       i=fib(n-1);

       #pragma omp task shared(j) firstprivate(n) if (n - 2 > THRESH)
       j=fib(n-2);

       if (n - 1 > THRESH) {
       #pragma omp taskwait 
       }
    
       return i+j;
    }
}

int main()
{
  int n = 30;

  omp_set_dynamic(0);
  omp_set_num_threads(4);

  #pragma omp parallel shared(n)
  {
    #pragma omp single
    printf ("fib(%d) = %d\n", n, fib(n));
  }
  return 0;
}
