#!/bin/bash
qsub -A PEACEndStation_2 -t 10 -n 1 --mode script ./runjob.sh
