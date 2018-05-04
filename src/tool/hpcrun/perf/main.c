#include <stdlib.h>
#include <string.h>

#define sizeof_type_instances(type, instances) (instances * sizeof(type))
#define malloc_type(type, instances) (type *) malloc(sizeof_type_instances(type, instances))

#define REPS 10
#define N 1000000

int *array;

int main(int argc, char **argv)
{
  int i;

  array = malloc_type(int, N);

  for (i = 0; i < REPS; i++) {
    memset(array, 0, sizeof_type_instances(int, N));
  }

  return 0;
}
