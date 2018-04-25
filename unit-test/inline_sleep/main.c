#include <stdio.h>

#include "inline_f.h"


int main()
{
  int stop = 2;
  int sum  = 0;

  sum = f(stop);
  printf("sum of %d: %d\n", sum, stop);
}

