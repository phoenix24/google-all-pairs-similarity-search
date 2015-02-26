CFLAGS = -DNDEBUG -I../sparsehash-read-only/src -O3 -D_FILE_OFFSET_BITS=64
SFLAGS = -shared -fPIC
CC = g++

.SUFFIXES: .o .cc

BIN = bin
SRC = src

LIBS =
LINKFLAGS =
PYTHON_INCLUDES = $(shell python-config --includes)

CPP_FILES = $(wildcard $(SRC)/*.cc)
OBJ_FILES = $(addprefix bin/,$(notdir $(CPP_FILES:.cc=.o)))

all: setup $(OBJ_FILES) 
	$(CC) $(CFLAGS) $(LINKFLAGS) -o ap $(OBJ_FILES) $(LIBS)

bin/%.o: src/%.cc
	$(CC) $(CFLAGS) -c -o $@ $<

python: $(CPP_FILES) $(SRC)/allpairs.i
	swig -c++ -python -o $(SRC)/allpairs_wrap.cc $(SRC)/allpairs.i

setup:
	mkdir -p $(BIN)

clean:
	rm -r ap *.o *.so *.pyc bin/ dist build MANIFEST || true
