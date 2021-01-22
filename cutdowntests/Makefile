FTDIR = /home/ywu/ddp/userspace_tree
# FTDIR = /home/space-tree

CC=clang++
CFLAGS= -I$(FTDIR)/CutDownPerconaFT/build/buildheader/ -I$(FTDIR)/CutDownPerconaFT/build/ -I$(FTDIR)/CutDownPerconaFT/build/portability/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/portability/ -I$(FTDIR)/CutDownPerconaFT/prefix/include/ft/ -I$(FTDIR)/CutDownPerconaFT/portability -I$(FTDIR)/CutDownPerconaFT -I$(FTDIR)/CutDownPerconaFT/prefix/include/util/ -I$(FTDIR)/CutDownPerconaFT/util/
LIBS = -L$(FTDIR)/CutDownPerconaFT/build/ft/ -L$(FTDIR)/CutDownPerconaFT/build/portability/

all: ftstar ftstar2 ftstar4 ftread ftreadnob ftsimple ftverify ftverify2

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
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -Wl,-rpath,$(FTDIR)/CutDownPerconaFT/build/ft -Wl,-rpath,$(FTDIR)/CutDownPerconaFT/build/portability -lft -ltokuportability

ftverify2: ftverify2.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftreadnob: ftreadnob.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

ftsimple: ftsimple.cc
	$(CC) $(CFLAGS) $(LIBS) $< -o $@ -lft -ltokuportability

clean:
	rm -f ftstar ftstar2 fstar4 ftread ftreadnob ftsimple ftverify ftverify2
