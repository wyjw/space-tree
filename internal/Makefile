CC=clang
CFLAGS=-I.
DEPS = ftstar.h

all: ftstar

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o
