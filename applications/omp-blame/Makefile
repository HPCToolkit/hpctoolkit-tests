CC=/usr/bin/gcc
OMPT_DIR=/home/johnmc/release-planning/openmp/openmp-install/lib
COMPILE=gcc -g -fopenmp -o omp-blame omp-blame.c -L$(OMPT_DIR) -lgomp

omp-blame: omp-blame.c
	$(COMPILE)

omp-blame-static:
	hpclink gcc -g -static -fopenmp -o omp-blame omp-blame.c -lomp

clean:
	/bin/rm -rf omp-blame hpctoolkit-* *.hpcstruct

dbclean:
	/bin/rm -rf hpctoolkit-*

omp-blame.hpcstruct:
	hpcstruct omp-blame

run: omp-blame omp-blame.hpcstruct
	/bin/rm -rf hpctoolkit-omp-blame-*
	hpcrun -e REALTIME -e OMP_IDLE -t ./omp-blame
	hpcprof -S omp-blame.hpcstruct hpctoolkit-omp-blame-measurements
	hpcviewer hpctoolkit-omp-blame-database
	# /bin/rm -rf hpctoolkit-omp-blame-*
