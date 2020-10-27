CC=clang
CLAGS = -Wall -std=c99 -O0 -g -Wconversion -Wextra

all: clean treenvme_ioctl

treenvme_ioctl: treenvme_ioctl.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o treenvme_ioctl
