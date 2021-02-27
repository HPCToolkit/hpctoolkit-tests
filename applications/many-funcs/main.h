#ifdef ONE_SECTION
#include "definitions-one-section.i"
#else
#include "definitions-many-sections.i"
#endif

#define N 10

int main(int argc, char **argv)
{
  int i;
  for (i=0; i < N; i++) {
#include "compute.i"
  }
  return 0;
}

