CC=clang
CFLAGS = -I/usr/include/linux/treenvme_ioctl.h -Wall -std=c99 -O0 -g -Wconversion -Wextra

all: clean treenvme_ioctl ioctl_struct

treenvme_ioctl: treenvme_ioctl.o
	$(CC) -o $@ $^ $(CFLAGS)

ioctl_struct: ioctl_struct.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o treenvme_ioctl ioctl_struct
