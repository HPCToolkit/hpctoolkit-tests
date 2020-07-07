HOST_FLAGS= -g -mcmodel=large  

many-funcs: many-funcs.c 
	time gcc -O0 -o many-funcs $(HOST_FLAGS) many-funcs.c

compute.hpp: compute.h macros.h Makefile
	gcc -E compute.h | sed 's/@/\n    /g' > compute.hpp

definitions.hpp: compute.h macros.h Makefile
	gcc -E definitions.h | tr '@' '\n' > definitions.hpp

many-funcs.c: main.h definitions.hpp compute.hpp Makefile
	gcc -E main.h | grep -v '#' > many-funcs.c

clean:
	/bin/rm -rf compute.hpp definitions.hpp many-funcs.c many-funcs *.hpcstruct

run: many-funcs
	hpcstruct many-funcs
