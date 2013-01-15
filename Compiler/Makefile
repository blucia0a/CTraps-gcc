#GXX=${HOME}/cvsandbox/GCC-experimental/inst/bin/g++
#GCC=${HOME}/cvsandbox/GCC-experimental/inst/bin/gcc

GMPDIR=${HOME}/cvsandbox/GCC-experimental/prereqs/GMP/inst
GCCSRCDIR=${HOME}/cvsandbox/GCC-experimental/inst/lib/gcc/x86_64-unknown-linux-gnu/4.7.0/../../../..
#GXX=g++-4.7
#GCC=gcc-4.7

UNAME:= $(shell uname -s)

RWCALLS=ReadWriteCalls.c
RWCALLSBIN=ReadWriteCalls.c.o


ifneq "$(RDTYPE)" ""
RDOPTS=-D$(RDTYPE)
endif

FOO:= $(shell echo $(UNAME))
GCCPLUGINS_DIR:= $(shell $(GCC) -print-file-name=plugin)
CFLAGS+= -I${GMPDIR}/include/ -I${GCCSRCDIR}/include -I$(GCCPLUGINS_DIR)/include -fPIC -O3 -g $(RDOPTS)

LDFLAGS+= -shared 

all: RWInst.so

%.o: %.cpp
	$(GCC) $(CFLAGS) -c $^ -o $@

%.o: %.c
	$(GCC) $(CFLAGS) -c $^ -o $@

ReadWriteCalls.c.o: ReadWriteCalls.c 
	$(GCC) $(CFLAGS) -c $^ -o $@

RWInst.o: RWInst.c
	$(GCC) $(CFLAGS) -c $^ -o $@

RWInst.so: RWInst.o $(RWCALLSBIN) 
	$(GCC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	-rm ReadWriteCalls.c.o RWInst.so RWInst.o
