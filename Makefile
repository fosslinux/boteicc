CFLAGS=-std=c11 -g -fno-common

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

chibicc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): chibicc.h

test/%.exe: test/%.c test/common.M1
	./chibicc -o test/$*.M1 test/$*.c
	blood-elf --little-endian --file test/$*.M1 --output tmp-elf.M1
	# This ordering is VITALLY important
	M1 --file x86_defs.M1 --file libc-core.M1 --little-endian --architecture x86 --file test/common.M1 --file test/$*.M1 --file tmp-elf.M1 --output tmp.hex2
	hex2 --file ELF-x86-debug.hex2 --file tmp.hex2 --output $@ --architecture x86 --base-address 0x8048000 --little-endian

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf chibicc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean
