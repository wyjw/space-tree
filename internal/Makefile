FTDIR = /home/ywu/ddp/userspace_tree

CC=clang++
CFLAGS= -I$(FTDIR)/PerconaFT/build/buildheader/ -I$(FTDIR)/PerconaFT/build/ -I$(FTDIR)/PerconaFT/prefix/include/ -I$(FTDIR)/PerconaFT/prefix/include/portability/ -I$(FTDIR)/PerconaFT/prefix/include/ft/ -I$(FTDIR)/PerconaFT/portability -I$(FTDIR)/PerconaFT -I$(FTDIR)/PerconaFT/prefix/include/util/ -I$(FTDIR)/PerconaFT/util/
LIBS = -L$(FTDIR)/PerconaFT/build/ft/ -L$(FTDIR)/PerconaFT/build/portability/

all: ftstar fttest fttest_bench fttest_block_alloc

ftstar: ftstar.cc ftstar.hpp
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

#ftstar: $(OBJ)
#	$(CC) -o ftstar $(LIBS) $^ -v 

fttest: fttest.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

fttest_bench: fttest_bench.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

fttest_block_alloc: fttest_block_alloc.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

clean:
	rm -f ftstar fftest fttest_block_alloc fftest_bench *.o
