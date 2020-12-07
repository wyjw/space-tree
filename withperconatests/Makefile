FTDIR = /home/ywu/ddp/userspace_tree
# FTDIR = /home/space-tree

CC=clang++
CFLAGS= -I$(FTDIR)/PerconaInBlockFT/build/buildheader/ -I$(FTDIR)/PerconaInBlockFT/build/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/portability/ -I$(FTDIR)/PerconaInBlockFT/prefix/include/ft/ -I$(FTDIR)/PerconaInBlockFT/portability -I$(FTDIR)/PerconaInBlockFT -I$(FTDIR)/PerconaInBlockFT/prefix/include/util/ -I$(FTDIR)/PerconaInBlockFT/util/
LIBS = -L$(FTDIR)/PerconaInBlockFT/build/ft/ -L$(FTDIR)/PerconaInBlockFT/build/portability/

all: ftstar ftstar2 ftstar4 ftread ftreadnob ftsimple ftverify

ftstar: ftstar.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftstar2: ftstar2.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

#ftstar3: ftstar3.cc
#	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftstar4: ftstar4.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftread: ftread.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftverify: ftverify.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftreadnob: ftreadnob.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftsimple: ftsimple.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

clean:
	rm -f ftstar
