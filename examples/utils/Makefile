SHELL = /bin/bash

CXX=c++
CXXFLAGS=-std=c++11
OBJ_DIR=obj
#SOURCE FILES
SOURCE=$(wildcard src/*.cpp)

#HEADER FILES
HEADER=$(wildcard include/*.h)

ifeq ($(BUILD_TYPE),debug)
	CXXFLAGS+= -Wall
endif

all: libutils.a libutils.so

.PHONY: all examples

libutils.a: $(SOURCE) $(HEADER)
	@ echo -e '\n[\033[0;33mUtils\033[0m]: ##Building Utils Object File and Static Library'
	$(CXX) $(CXXFLAGS) -r $(foreach src,$(SOURCE),$(src)) -o $(@:.a=).o
	@ ar cr $@ $(@:.a=).o
	@ rm -f *.o

libutils.so: $(SOURCE) $(HEADER)
	@ echo -e '\n[\033[0;33mUtils\033[0m]: ##Building Utils Object File and Dynamic Library'
	$(CXX) $(CXXFLAGS) -fPIC -shared $(foreach src,$(SOURCE),$(src)) -o $(@:.so=).so


examples: libutils.a make_examples

make_examples:
	$(MAKE) -C examples

clean: 
	rm -f *.a
	rm -f *.so