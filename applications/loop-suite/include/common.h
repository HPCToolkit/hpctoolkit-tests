#include <stdio.h>
#include <stdlib.h>

#ifdef MPI
#include <mpi.h>
#endif

#define N (1L << 30)

#ifdef MPI
void test_mpi(int world_size, int world_rank, long n);
#else
void test(long n);
#endif

void loop(long n)
{
   volatile long i;
   for(i = 0; i < n; i++);
}

long parse(int argc, char **argv)
{
   long n;
   if (argc != 2) n = N;
   else n = atol(argv[1]);

   if (n < 0) {
     printf("usage: loop [n], where n > 0 && n < 2^64\n"); 
     exit(-1);
   }

   printf("argument = %ld \n", n); 

   return n;
}

int main(int argc, char **argv)
{
#ifdef MPI
   MPI_Init(&argc, &argv);

   int world_size;
   MPI_Comm_size(MPI_COMM_WORLD, &world_size);

   int world_rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

#endif

   long n = parse(argc, argv);

#ifdef MPI
   test_mpi(world_size, world_rank, n);
#else
   test(n);
#endif

#ifdef MPI
   MPI_Finalize();
#endif

   return 0;
}
