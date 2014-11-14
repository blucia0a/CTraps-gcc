build:
	gcc-4.8 -fplugin=$(PWD)/../Compiler/RWInst.so ../Test/test.c -O1 -g -o ../Test/test -L $(PWD)/../Runtime/ -lRWCalls

run: build
	-DYLD_LIBRARY_PATH=$(PWD)/../Runtime ./test

clean:
	-rm -rf ./test ./test.dSYM
