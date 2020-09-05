CC=clang++
CFLAGS=-I/home/ywu/ddp/userspace_tree/PerconaFT/build/buildheader/ -I/home/ywu/ddp/userspace_tree/PerconaFT/prefix/include/
DEPS = ftstar.hpp
LIBS = -L/home/ywu/ddp/userspace_tree/PerconaFT/build/ft/
OBJ = ftstar.o

%.o: %.cc $(DEPS)
	$(CC) -o $@ $< -static $(CFLAGS) $(LIBS) -lft

ftstar: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f ftstar *.o
