#!/bin/bash
#COBALT -t 20 
#COBALT -n 1
#COBALT -A CSC250STTO11
echo "Starting Cobalt job script"
export n_nodes=$COBALT_JOBSIZE
export n_mpi_ranks_per_node=1
export n_mpi_ranks=$(($n_nodes * $n_mpi_ranks_per_node))
aprun -n $n_mpi_ranks -N $n_mpi_ranks_per_node \
  -cc depth \
  ./vdso-query 
