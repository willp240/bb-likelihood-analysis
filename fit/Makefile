CXX=g++
ROOT_FLAGS=`root-config --cflags --libs `
G4_FLAGS=`geant4-config --cflags --libs`

GSL_FLAGS=-lgsl

OXSX_ROOT = /home/kroupova/oxsx
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

all: bin/make_pdfs bin/make_trees bin/split_data bin/fit_dataset bin/up_count_lim bin/build_azimov bin/split_half

bin/fit_dataset: fit_dataset.cc $(LIB)
	mkdir -p bin
	$(CXX)  fit_dataset.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) -larmadillo -o $@


bin/up_count_lim: up_count_lim.cc $(LIB)
	mkdir -p bin
	$(CXX)  up_count_lim.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) $(GSL_FLAGS) -larmadillo -o $@

bin/make_pdfs: make_pdfs.cc $(LIB)
	mkdir -p bin
	$(CXX)  make_pdfs.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) -larmadillo -o $@

bin/split_data: split_data.cc $(LIB)
	mkdir -p bin
	$(CXX)  split_data.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) -larmadillo -o $@

bin/split_half: split_half.cc $(LIB)
	mkdir -p bin
	$(CXX)  split_half.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) $(GSL_FLAGS)  -larmadillo -o $@

bin/make_trees: make_trees.cc $(LIB)
	mkdir -p bin
	$(CXX)  make_trees.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) -larmadillo -o $@


bin/build_azimov: build_azimov.cc $(LIB)
	mkdir -p bin
	$(CXX)  build_azimov.cc -I$(INC_DIR) -I$(OXSX_INC) -w -L$(LIB_DIR) -L$(OXSX_LIB_DIR) -l$(LIB_NAME) -l$(OXSX_LIB_NAME)  $(ROOT_FLAGS) $(G4_FLAGS) $(H5_LIBS) -larmadillo -o $@

$(LIB) : $(OBJ_FILES)
	mkdir -p $(LIB_DIR)
	ar rcs  $@ $^

build/%.o : src/%.cc
	mkdir -p build
	$(CXX) -c -w $< -I$(OXSX_INC) -Isrc/ -w $(ROOT_FLAGS) $(G4_FLAGS) -o $@

install:
	ln -sf `readlink -f bin/make_pdfs` $(PREFIX)
	ln -sf `readlink -f bin/make_trees` $(PREFIX)
	ln -sf `readlink -f bin/split_data` $(PREFIX)
	ln -sf `readlink -f bin/split_half` $(PREFIX)
	ln -sf `readlink -f bin/fit_dataset` $(PREFIX)
	ln -sf `readlink -f bin/fit_dataset_batch` $(PREFIX)
	ln -sf `readlink -f bin/up_count_lim` $(PREFIX)
	ln -sf `readlink -f bin/build_azimov` $(PREFIX)
	chmod +x bin/make_pdfs
	chmod +x bin/make_trees
	chmod +x bin/split_data
	chmod +x bin/split_half
	chmod +x bin/fit_dataset
	chmod +x bin/fit_dataset_batch
	chmod +x bin/up_count_lim
	chmod +x bin/build_azimov

clean:
	rm -f bin/make_pdfs
	rm -f bin/make_trees
	rm -f bin/split_data
	rm -f bin/fit_dataset
	rm -f bin/up_count_lim
	rm -f bin/build_azimov
	rm -f bin/split_half

	rm -f build/*.o
	rm -f lib/libbbfit.a
	rm -f $(PREFIX)/make_pdfs
	rm -f $(PREFIX)/make_trees
	rm -f $(PREFIX)/split_data
	rm -f $(PREFIX)/fit_dataset
	rm -f $(PREFIX)/up_count_lim
	rm -f $(PREFIX)/split_half
	rm -f $(PREFIX)/build_azimov
