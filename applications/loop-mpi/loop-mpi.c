#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N (1L << 30)

int main(int argc, char **argv)
{
   long n;
   volatile long i;

   MPI_Init(&argc, &argv);

   int world_size;
   MPI_Comm_size(MPI_COMM_WORLD, &world_size);

   int world_rank;
   MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

   n = N; 

   if (n < 0) {
     printf("usage: loop [n], where n > 0 && n < 2^64\n"); 
     exit(-1);
   }

   printf("test: begin %ld iterations on rank %d\n", n, world_rank); 

   for(i = 0; i < n; i++);

   printf("test: end %ld iterations on rank %d\n", n, world_rank); 

   MPI_Finalize();

   return 0;
}
