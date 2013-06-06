CXXFLAGS = -O0 -Wall -Werror -Wno-unused-function -std=c++11
CXXFLAGS += -ggdb
LDFLAGS += -pthread

# Libfreetype
CXXFLAGS += $(shell freetype-config --cflags)
LDFLAGS += $(shell freetype-config --libs)

OBJS = bdf_import.o freetype_import.o importtools.o \
	datafile.o encode.o optimize.o c_export.o

all: run_unittests compress

compress: main.o $(OBJS)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

unittests.cc: *.hh
	cxxtestgen --have-eh --error-printer -o unittests.cc $^

unittests: unittests.cc $(OBJS)
	g++ $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

run_unittests: unittests
	./unittests

%.o: %.cc *.hh
	g++ $(CXXFLAGS) -c $<