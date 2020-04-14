#!/bin/bash
hpcrun -e cycles -e OMP_IDLE -o hpctoolkit-freq-measurements ./fib
hpcrun -e cycles@1000000 -e OMP_IDLE -o hpctoolkit-period-measurements ./fib
hpcprof -S fib.hpcstruct  -o hpctoolkit-period-database hpctoolkit-period-measurements 
hpcprof -S fib.hpcstruct  -o hpctoolkit-freq-database hpctoolkit-freq-measurements 
tar cf foo.tar  hpctoolkit-freq-database  hpctoolkit-period-database
scp foo.tar gpu.cs.rice.edu:
