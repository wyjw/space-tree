FTDIR = /home/ywu/ddp/userspace_tree
# FTDIR = /home/space-tree

CC=clang++
CFLAGS= -I$(FTDIR)/PerconaFT/build/buildheader/ -I$(FTDIR)/PerconaFT/build/ -I$(FTDIR)/PerconaFT/prefix/include/ -I$(FTDIR)/PerconaFT/prefix/include/portability/ -I$(FTDIR)/PerconaFT/prefix/include/ft/ -I$(FTDIR)/PerconaFT/portability -I$(FTDIR)/PerconaFT -I$(FTDIR)/PerconaFT/prefix/include/util/ -I$(FTDIR)/PerconaFT/util/
LIBS = -L$(FTDIR)/PerconaFT/build/ft/ -L$(FTDIR)/PerconaFT/build/portability/

all: ftstar fttest fttest_bench fttest_block_alloc ft_serialize kv_bench kv_bench2 kv_bench3

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

ft_serialize: ft_serialize.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

kv_bench: kv_bench.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

kv_bench2: kv_bench2.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

kv_bench3: kv_bench3.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability
clean:
	rm -f ftstar fftest ft_serialize fttest_block_alloc fftest_bench kv_bench *.o *.ft
