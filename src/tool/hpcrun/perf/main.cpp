#include <stdlib.h>
#include <string.h>

#define sizeof_type_instances(type, instances) (instances * sizeof(type))
#define malloc_type(type, instances) (type *) malloc(sizeof_type_instances(type, instances))

#define REPS 10
#define N (1024 * 1024)


template<int reps, size_t n>
void
do_set(int *array)
{
  for (int i = 0; i < reps; i++) {
    memset(array, 0, n);
  }
}


template<int reps, size_t n>
void
do_init()
{
  int *array = malloc_type(int, n);

  // a warm up iteration
  memset(array, 0, sizeof_type_instances(int, n));

  do_set<reps,sizeof_type_instances(int, n)>(array);

  free(array);
}


int 
main(int argc, char **argv)
{
  do_init<REPS, N << 0>();
  do_init<REPS, N << 1>();
  do_init<REPS, N << 2>();
  do_init<REPS, N << 3>();
  do_init<REPS, N << 4>();
  do_init<REPS, N << 5>();
  do_init<REPS, N << 6>();
  do_init<REPS, N << 7>();
  do_init<REPS, N << 8>();
  do_init<REPS, N << 9>();
  do_init<REPS, N << 10>();

  return 0;
}
