TARGETS =  \
	hello-c  \
	hello-f90 \
	hello-f 

RUN=srun

ifdef R
  NPE=-n $(R)
endif

all: $(TARGETS)

hello-c: hello-c.c
	mpicc -o $@ $<

hello-f90: hello-f90.f90
	mpif90 -o $@ $<

hello-f: hello-f.f
	mpif77 -o $@ $<

clean:
	/bin/rm -f $(TARGETS) hello-c.o hello-f.o hello-f90.o

run:
	@echo "make run R=number of ranks (1 <= R <= 12)"
	$(RUN) $(NPE) hello-c
	$(RUN) $(NPE) hello-f90
	$(RUN) $(NPE) hello-f
