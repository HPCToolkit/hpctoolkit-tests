#!/bin/sh
echo "Starting Cobalt job script"
runjob -np 1 -p 1 --verbose=INFO  make check >output.txt
