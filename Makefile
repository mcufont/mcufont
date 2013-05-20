CXXFLAGS = -O2 -Wall -Werror -Wno-unused-function -std=c++11
CXXFLAGS += -ggdb

OBJS = bdffont.o datafile.o encode.o optimize.o

all: run_unittests compress

compress: main.o $(OBJS)
	g++ $(CXXFLAGS) -o $@ $^

unittests.cc: *.hh
	cxxtestgen --have-eh --error-printer -o unittests.cc $^

unittests: unittests.cc $(OBJS)
	g++ $(CXXFLAGS) -o $@ $^

run_unittests: unittests
	./unittests

%.o: %.cc *.hh
	g++ $(CXXFLAGS) -c $<