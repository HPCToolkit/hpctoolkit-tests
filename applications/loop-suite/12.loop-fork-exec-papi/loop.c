#include "../include/common.h"

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

void parent(long n)
{
  loop(n);
}

void child(long n)
{
  char arg[128];
  sprintf(arg, "%ld n");

  char *const argv[] = { arg, 0 };
  execv("./child", argv);
}

void test(long n)
{
  if (fork()) { 
    parent(n);
  } else {
    child(n);
  }
}
