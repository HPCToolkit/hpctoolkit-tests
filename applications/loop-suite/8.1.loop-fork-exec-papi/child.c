#include "../include/common.h"

void exec_child(long n)
{
  loop(n);
}
void test(long n)
{
  exec_child(n);
}
