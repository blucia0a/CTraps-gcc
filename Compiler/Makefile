GCC=gcc-4.8
GXX=g++-4.8
LIBIBERTYDIR=$(shell brew --prefix)/lib/x86_64
GMPDIR=$(shell brew --prefix)/Cellar/gmp4/4.3.2

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
	$(GXX) $(CFLAGS) -c $^ -o $@

%.o: %.c
	$(GXX) $(CFLAGS) -c $^ -o $@

ReadWriteCalls.c.o: ReadWriteCalls.c 
	$(GXX) $(CFLAGS) -c $^ -o $@

RWInst.o: RWInst.c
	$(GXX) $(CFLAGS) -c $^ -o $@

RWInst.so: RWInst.o $(RWCALLSBIN) 
	$(GXX) -undefined dynamic_lookup $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	-rm ReadWriteCalls.c.o RWInst.so RWInst.o
