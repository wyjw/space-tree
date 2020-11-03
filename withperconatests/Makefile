FTDIR = /home/ywu/ddp/userspace_tree
# FTDIR = /home/space-tree

CC=clang++
CFLAGS= -I$(FTDIR)/PerconaInBlockFT/build/buildheader/ -I$(FTDIR)/PerconaInBlockFT/build/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/portability/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/ft/ -I$(FTDIR)/PerconaInBlockFT/portability -I$(FTDIR)/PerconaInBlockFT -I$(FTDIR)/PerconaInBlockFT/prefix/include/util/ -I$(FTDIR)/PerconaInBlockFT/util/
LIBS = -L$(FTDIR)/PerconaInBlockFT/build/ft/ -L$(FTDIR)/PerconaInBlockFT/build/portability/

all: ftstar ftstar2

ftstar: ftstar.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftstar2: ftstar2.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability
clean:
	rm -f ftstar
