ARCH=sm_70

HOST_FLAGS= -g -Xcompiler -mcmodel=large  
GPU_FLAGS=-lineinfo -arch $(ARCH) 

main: many-cuda-funcs.cu 
	time nvcc -O0 -o main $(HOST_FLAGS) $(GPU_FLAGS) many-cuda-funcs.cu

compute.hpp: compute.h macros.h Makefile
	gcc -E compute.h | sed 's/@/\n    /g' > compute.hpp

definitions.hpp: compute.h macros.h Makefile
	gcc -E definitions.h | tr '@' '\n' > definitions.hpp

many-cuda-funcs.cu: main.h definitions.hpp compute.hpp Makefile
	gcc -E main.h | grep -v '#' > many-cuda-funcs.cu

clean:
	/bin/rm -rf compute.hpp definitions.hpp many-cuda-funcs.cu *.cubin main *.hpcstruct

many-cuda-funcs.$(ARCH).cubin : main
	cuobjdump -xelf all main

run: many-cuda-funcs.$(ARCH).cubin 
	hpcstruct $<
	
