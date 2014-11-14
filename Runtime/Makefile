GXX=g++-4.8
GCC=gcc-4.8

CFLAGS+= -g -fPIC -O3  -pthread  -Wno-deprecated
CXXFLAGS+= -g -fPIC -O3  -pthread -std=c++11 -Wno-deprecated

LDFLAGS+= -shared -std=c++11 -lpthread -ldl

all: libRWCalls.so

%.o: %.cpp
	$(GXX) $(CXXFLAGS) -c $^ -o $@

%.o: %.c
	$(GCC) $(CFLAGS) -c $^ -o $@

RWCalls.s: RWCalls.cpp
	$(GXX) $(CXXFLAGS) -S $^ -o $@

libRWCalls.so: RWCalls.o  thd_ctr.o comm_graph.o
	$(GXX) -I. $(CFLAGS) $(LDFLAGS) $^ -o $@ -ldl -Wno-deprecated

clean:
	-rm libRWCalls.so 
	-rm *.o 
