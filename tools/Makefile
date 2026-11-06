default: all

all: spz2tiny3d

load-spz.o: ../spz/src/cc/load-spz.cc
splat-c-types.o: ../spz/src/cc/splat-c-types.cc
splat-types.o: ../spz/src/cc/splat-types.cc

CXXFLAGS += -I../spz/src/cc/ -I.

%.o: ../spz/src/cc/%.cc
	$(CXX) -c -o $@ $<

spz2tiny3d: spz2tiny3d.o load-spz.o splat-c-types.o splat-types.o
	$(CXX) -o $@ $^ -lzstd -lz
