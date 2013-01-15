GXX=${HOME}/cvsandbox/GCC-experimental/inst/bin/g++
GCC=${HOME}/cvsandbox/GCC-experimental/inst/bin/gcc

CTRAPSHOME=${HOME}/cvsandbox/CommGraph/Runtime

CFLAGS+= -fPIC -O3 -g -pthread  -I${HOME}/cvsandbox/GCC-experimental/inst/include  -I${CTRAPSHOME} -Wno-deprecated
CXXFLAGS+= -fPIC -O3 -g -pthread -std=c++11 -I${HOME}/cvsandbox/GCC-experimental/inst/include  -I${CTRAPSHOME} -Wno-deprecated

LDFLAGS+= -shared -std=c++11 -lpthread -L${HOME}/cvsandbox/GCC-experimental/inst/lib -L${HOME}/cvsandbox/GCC-experimental/inst/lib64 -L${HOME}/cvsandbox/GCC-experimental/inst/libexec -ldl

all: null_plugin.so sfi_plugin.so cgtest_plugin.so cganom_plugin.so

%.o: %.cpp
	$(GXX) $(CXXFLAGS) -c $^ -o $@

%.o: %.c
	$(GCC) $(CFLAGS) -c $^ -o $@

%.so: %.o
	$(GXX) -I. $(CFLAGS) $(LDFLAGS) $^ -o $@ -Wno-deprecated

clean:
	-rm *.so 
	-rm *.o 
