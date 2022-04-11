SHELL = /bin/bash

CXX = c++
CXXFLAGS = -std=c++11
BIN = ../bin

#SOURCE FILES
SOURCE = $(wildcard *.cpp)
PROGS = $(SOURCE:.cpp=)
OBJ = $(SOURCE:.cpp=.o)

ifeq ($(BUILD_TYPE),debug)
	CXXFLAGS += -Wall
endif

all: dir copy clean

.PHONY: all

dir:
	@echo " ##Creating Binary Directories "
	@if [ ! -d $(BIN) ]; then mkdir $(BIN); fi

%.o: %.cpp 
	@echo " ##Compiling Example $*.cpp"
	$(CXX) $(CXXFLAGS) $*.cpp -o $* $(LUTILS)

%.o: LUTILS = -L../ -lutils

copy: $(OBJ)
	@cp -f $(PROGS) $(BIN)

clean:
	rm -f $(PROGS)


