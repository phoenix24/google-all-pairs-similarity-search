CFLAGS= -DNDEBUG -I../sparsehash-read-only/src -O3 -D_FILE_OFFSET_BITS=64

SFLAGS= -shared -fPIC

CC = g++

.SUFFIXES: .o .cc

LIBS =

LINKFLAGS =

PYTHON_INCLUDES = $(shell python-config --includes)

OBJS_c = allpairs.cc data-source-iterator.cc
OBJS_o = $(OBJS_c:.cc=.o)

ap: $(OBJS_c) $(OBJS_o) main.cc main.o
	$(CC) $(CFLAGS) $(LINKFLAGS) -o ap main.o $(OBJS_o) $(LIBS)

data-source-iterator.o: data-source-iterator.h

allpairs.o: allpairs.h

main.o: $(OBJS_o)

python-swig:
	swig -c++ -python allpairs.i
	$(CC) $(CFLAGS) $(SFLAGS) $(PYTHON_INCLUDES) -c allpairs.cc data-source-iterator.cc allpairs_wrap.cxx
	$(CC) $(CFLAGS) $(SFLAGS) allpairs.o data-source-iterator.o allpairs_wrap.o -o _allpairs.so

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm ap *.o *.so *.pyc
