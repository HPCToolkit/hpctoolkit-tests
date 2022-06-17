#include "../include/common.h"

#include <sys/types.h>
#include <unistd.h>

void test(long n)
{
#pragma omp parallel num_threads(2) 
{
  loop(n);
}
}
