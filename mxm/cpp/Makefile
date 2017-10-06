CC=xlc++
CFLAGS=-g -O2

OBJ = mxm.cpp

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mxm.exe: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o mxm.exe
