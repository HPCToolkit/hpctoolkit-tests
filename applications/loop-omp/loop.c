#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define N (1L << 30)

int main(int argc, char **argv)
{
   long n;

   if (argc != 2) n = N;
   else n = atol(argv[1]);

   if (n < 0) {
     printf("usage: loop [n], where n > 0 && n < 2^64\n"); 
     exit(-1);
   }

#pragma omp parallel
{
   volatile long i;
   printf("thread %d executes %ld iterations\n", omp_get_thread_num(), n); 
   for(i = 0; i < n; i++);
}

   return 0;
}
