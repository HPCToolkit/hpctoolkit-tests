##
##  Copyright (c) 2017, Rice University.
##  See the file LICENSE for details.
##
##  Mark W. Krentel
##  May 2017
##
##  Build map-sum and libtrace.so with autotools and libtool.  This
##  more closely reflects the conditions in hpcrun.
##

bin_PROGRAMS = map-sum
map_sum_SOURCES = map-sum.cpp

lib_LTLIBRARIES = libtrace.la
libtrace_la_SOURCES = monitor.c realtime.c trace.cpp
libtrace_la_LDFLAGS = -ldl -lrt

bin_SCRIPTS = run.sh

