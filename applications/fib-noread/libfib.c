#include "libfib.h"

long libfib(int n, fibfn_t fib)
{
  if (n < 2) return n;
  return fib(n-1) + fib(n-2);
}
