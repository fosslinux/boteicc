CFLAGS=-std=c11 -g -fno-common

chibicc: main.o codegen.o parse.o tokenize.o type.o util.o bootstrappable.o
	$(CC) -o chibicc $^ $(LDFLAGS)

test: chibicc
	./test.sh
	./test-driver.sh

clean:
	rm -f chibicc *.o *~ tmp*

.PHONY: test clean
