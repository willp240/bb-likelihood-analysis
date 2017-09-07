CXX=g++
ROOT_FLAGS=`root-config --cflags --libs `
G4_FLAGS=`geant4-config --cflags --libs`

OXSX_ROOT ?=
OXSX_INC=$(OXSX_ROOT)/include
OXSX_LIB_DIR=$(OXSX_ROOT)/build 
OXSX_LIB_NAME=oxsx

H5_LIBS = -lhdf5_hl_cpp -lhdf5_cpp -lhdf5_hl -lhdf5

SRC_FILES := $(wildcard src/*.cc)
OBJ_FILES := $(addprefix build/, $(notdir $(SRC_FILES:.cc=.o)))

PREFIX ?= /usr/local/bin

INC_DIR=src
LIB_DIR=lib
LIB_NAME=bbfit

LIB=$(LIB_DIR)/lib$(LIB_NAME).a

all: bin/make_pdfs bin/make_trees

bin/make_pdfs: make_pdfs.cc $(LIB)
	mkdir -p bin
	$(CXX)  make_pdfs.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) -larmadillo -o $@

bin/make_trees: make_trees.cc $(LIB)
	mkdir -p bin
	$(CXX)  make_trees.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) -larmadillo -o $@


$(LIB) : $(OBJ_FILES)
	mkdir -p $(LIB_DIR)
	ar rcs  $@ $^

build/%.o : src/%.cc
	mkdir -p build
	$(CXX) -c -w $< -I$(OXSX_INC) -Isrc/ -w $(ROOT_FLAGS) $(G4_FLAGS) -o $@

install:
	ln -sf `readlink -f bin/neufitter` $(PREFIX)
	chmod +x bin/neufitter

clean:
	rm -f bin/make_pdfs
	rm -f bin/make_trees
	rm -f build/*.o
	rm -f lib/libbbfit.a
	rm -f $(PREFIX)/neufitter
	rm -f $(PREFIX)/neufitter_batch
	rm -f $(PREFIX)/neufitter_recombine