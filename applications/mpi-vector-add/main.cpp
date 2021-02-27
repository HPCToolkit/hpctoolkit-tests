#include <mpi.h>
#include <stdio.h>

extern int vecadd();

int
main(int argc, char **argv)
{
   int myrank;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

   printf("Rank %d begin\n", myrank);

   vecadd();

   printf("Rank %d complete\n", myrank);

   MPI_Finalize();

    return 0;
}

