HOST_FLAGS= -g -mcmodel=large  

all: many-funcs-many-sections many-funcs-one-section

many-funcs-many-sections: many-funcs-many-sections.c 
	time gcc -O0 -o many-funcs-many-sections $(HOST_FLAGS) many-funcs-many-sections.c

many-funcs-one-section: many-funcs-one-section.c 
	time gcc -O0 -o many-funcs-one-section $(HOST_FLAGS) many-funcs-one-section.c

compute.i: compute.h macros.h Makefile
	gcc -E compute.h | sed 's/@/\n    /g' > compute.i

definitions-many-sections.i: compute.h macros.h Makefile
	gcc -E definitions.h | tr '@' '\n' > definitions-many-sections.i

definitions-one-section.i: compute.h macros.h Makefile
	gcc -E -DONE_SECTION definitions.h | tr '@' '\n' > definitions-one-section.i

many-funcs-many-sections.c: main.h definitions-many-sections.i compute.i Makefile
	gcc -E main.h | grep -v '^#' > many-funcs-many-sections.c

many-funcs-one-section.c: main.h definitions-one-section.i compute.i Makefile
	gcc -E -DONE_SECTION main.h | grep -v '^#' > many-funcs-one-section.c

clean:
	/bin/rm -rf compute.i definitions-*.i
	/bin/rm -rf many-funcs-one-section.c many-funcs-many-sections.c 
	/bin/rm -rf many-funcs-many-sections many-funcs-one-section *.hpcstruct

run: many-funcs-many-sections many-funcs-one-section
	hpcstruct many-funcs-many-sections
	hpcstruct many-funcs-one-section
