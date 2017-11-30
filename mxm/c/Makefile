include ../Makefile.include

OBJ = mxm.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mxm.exe: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

check:
	./mxm.exe 1000 1000 500
clean:
	rm -f *.o mxm.exe
