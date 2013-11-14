GXX=g++-4.7
GCC=gcc-4.7

CFLAGS+= -g -fPIC -O3  -pthread  -I${HOME}/cvsandbox/GCC-experimental/inst/include  -Wno-deprecated
CXXFLAGS+= -g -fPIC -O3  -pthread -std=c++11 -I${HOME}/cvsandbox/GCC-experimental/inst/include -Wno-deprecated

LDFLAGS+= -shared -std=c++11 -lpthread -L${HOME}/cvsandbox/GCC-experimental/inst/lib -L${HOME}/cvsandbox/GCC-experimental/inst/lib64 -L${HOME}/cvsandbox/GCC-experimental/inst/libexec -ldl

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
