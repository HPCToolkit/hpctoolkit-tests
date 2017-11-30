include ../Makefile.include

OBJ = mxm.cpp

%.o: %.cpp $(DEPS)
	$(CPP) -c -o $@ $< $(CFLAGS)

mxm.exe: $(OBJ)
	$(CPP) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o mxm.exe
