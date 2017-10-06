F90=xlf90
CFLAGS=-g -O2

OBJ = mxm.f90

%.o: %.f90 $(DEPS)
	$(F90) -c -o $@ $< $(CFLAGS)

mxm.exe: $(OBJ)
	$(F90) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o mxm.exe
