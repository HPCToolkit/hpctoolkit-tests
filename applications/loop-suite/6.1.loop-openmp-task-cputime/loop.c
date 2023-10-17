#include "../include/common.h"

#include <sys/types.h>
#include <unistd.h>

void taskroot(long n)
{
#pragma omp task
  loop(n);
#pragma omp task
  loop(n);
}

void test(long n)
{
#pragma omp parallel num_threads(2) 
{
  taskroot(n);
}
}
