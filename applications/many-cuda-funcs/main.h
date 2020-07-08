#include "definitions.hpp"

#define N 10

int main(int argc, char **argv)
{
  int i;
  for (i=0; i < N; i++) {
#include "compute.hpp"
  }
  return 0;
}

