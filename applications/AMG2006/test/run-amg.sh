#!/bin/bash

export OMP_NUM_THREADS=2
export OMP_WAIT_POLICY=active
export KMP_BLOCKTIME=infinite
mpirun  -n 4 hpcrun  -o hpctoolkit-all-measurements -e cycles -t ./amg2006 -P 2 2 1 -n 2 2 4  -r 10 10 10
