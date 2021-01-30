FTDIR = /home/ywu/ddp/userspace_tree
# FTDIR = /home/space-tree

CC=clang++
CFLAGS= -I$(FTDIR)/CutDownPerconaFT/build/buildheader/ -I$(FTDIR)/CutDownPerconaFT/build/ -I$(FTDIR)/CutDownPerconaFT/build/portability/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/portability/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/ft/ -I$(FTDIR)/CutDownPerconaFT/portability -I$(FTDIR)/CutDownPerconaFT -I$(FTDIR)/CutDownPerconaFT/prefix/include/util/ -I$(FTDIR)/CutDownPerconaFT/util/
LIBS = -L$(FTDIR)/CutDownPerconaFT/build/ft/ -L$(FTDIR)/CutDownPerconaFT/build/portability/

all: ftverify

ftverify: ftverify.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -Wl,-rpath,$(FTDIR)/CutDownPerconaFT/build/ft -Wl,-rpath,$(FTDIR)/CutDownPerconaFT/build/portability -lft -ltokuportability

clean:
	rm -f ftverify
