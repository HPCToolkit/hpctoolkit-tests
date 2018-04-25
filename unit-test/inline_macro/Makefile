CC=gcc
CFLAGS=-O0
LFLAGS=

all: inline


inline: inline_f.o main.o 
	$(CC) -g inline_f.o main.o -o inline $(LFLAGS)

%.o: %.c
	$(CC) -c -g $(CFLAGS) -o $@ $<

clean:
	rm -f *.o inline

