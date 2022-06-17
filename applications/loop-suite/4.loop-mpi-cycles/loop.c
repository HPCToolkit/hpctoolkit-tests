#include "../include/common.h"

#include <sys/types.h>
#include <unistd.h>

void test_mpi(int world_size, world_rank, long n)
{
  loop(n);
}
