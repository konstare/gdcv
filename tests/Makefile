CC=gcc
CFLAGS=$(pkg-config --cflags  zlib) -Wall  -Wextra  -pedantic -Wformat-security -Wfloat-equal -Wshadow -Wwrite-strings
LDLIBS=$(shell pkg-config --libs  zlib)
DEBUG= -O0 -g3 -ggdb -static-libasan   -fsanitize=address

VPATH=../

testsuite: testsuite.c test.h  utils.c dictzip.c utf8proc/utf8proc.c stardict.c dictd.c dsl.c dictionaries.c index.c cvec.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG)

.PHONY: clean test

test: testsuite
	./testsuite

clean:
	rm -f testsuite
