CC=clang++
CFLAGS=-I/home/ywu/ddp/userspace_tree/PerconaFT/build/buildheader/ -I/home/ywu/ddp/userspace_tree/PerconaFT/prefix/include/ -I/home/ywu/ddp/userspace_tree/PerconaFT/prefix/include/portability/
DEPS = ftstar.hpp
LIBS = -L/home/ywu/ddp/userspace_tree/PerconaFT/build/ft/
OBJ = ftstar.o

ftstar.o: ftstar.cc ftstar.hpp
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft

#ftstar: $(OBJ)
#	$(CC) -o ftstar $(LIBS) $^ -v 

testo: fttest.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft

clean:
	rm -f ftstar *.o
