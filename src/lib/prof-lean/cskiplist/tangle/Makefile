H= /home/johnmc/pkgs-src/hpctoolkit-openmp
H=/home/johnmc/pkgs-src/hpctoolkit-master
S=$(H)/src
P= $(S)/lib/prof-lean

L=$(H)/BUILD/src/lib/
PLL=$(L)/prof-lean/.libs/libHPCprof-lean.a

OFLAGS=-g -fopenmp

cskiplist-tangle: cskiplist-tangle.c Makefile
	gcc $(OFLAGS) -o $@ -I $(H) -I $(P) $< $(PLL)

clean:
	/bin/rm -rf cskiplist-tangle
