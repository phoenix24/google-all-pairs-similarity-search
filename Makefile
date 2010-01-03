CFLAGS= -DNDEBUG -I../google-sparsehash-read-only/src -O3 -D_FILE_OFFSET_BITS=64
#CFLAGS= -g -Isparsehash-0.7/src -D_FILE_OFFSET_BITS=64

CC = g++

.SUFFIXES: .o .cc

LIBS =

LINKFLAGS =

OBJS_c = allpairs.cc data-source-iterator.cc
OBJS_o = $(OBJS_c:.cc=.o)

ap: $(OBJS_c) $(OBJS_o) main.cc main.o
	$(CC) $(CFLAGS) $(LINKFLAGS) -o ap main.o $(OBJS_o) $(LIBS)

allpairs.o: allpairs.h

main.o: allpairs.o

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm ap *.o
