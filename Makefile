CC=gcc
CFLAGS=$(pkg-config --cflags  zlib) -Wall  -Wextra  -pedantic -Wformat-security -Wfloat-equal -Wshadow -Wwrite-strings #-Wconversion
OPTIMISATIONS= -O2  -march=native
LDLIBS=$(shell pkg-config --libs  zlib)
DEBUG_BASIC= -O0 -g3 -ggdb
DEBUG_GPROF =$(DEBUG_BASIC) -pg
DEBUG_VALGRIND= $(DEBUG_BASIC) -static-libasan
DEBUG= $(DEBUG_VALGRIND) -fsanitize=address
OBJECTS = gdcv.c  utils.c dictzip.c utf8proc/utf8proc.c dictionary_formats/stardict.c dictionary_formats/dictd.c dictionary_formats/dsl.c dictionaries.c index.c cvec.c forms.c buffer.c

OPTIONS=$(OPTIMISATIONS)

.PHONY : test

gdcv: $(OBJECTS)
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(OPTIONS)

debug: OPTIONS=$(DEBUG)
debug: gdcv

valgrind: OPTIONS=$(DEBUG_VALGRIND)
valgrind: gdcv
#	G_SLICE=always-malloc G_DEBUG=gc-friendly  valgrind -v --tool=memcheck --leak-check=full --num-callers=40 --log-file=valgrind.log --track-origins=yes ./$@ ../Dicts

gprof: OPTIONS=$(DEBUG_GPROF)
gprof: gdcv
#	./$@ ../Dicts
#	gprof  $@ gmon.out > prof_output

%.o: %.c
	$(CC)  -o $@ -c $< $(CFLAGS) $(LDLIBS) $(OPTIONS)

clean:
	rm -f  gdcv *.o
