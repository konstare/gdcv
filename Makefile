CC=gcc
CFLAGS=$(pkg-config --cflags  zlib) -Wall  -Wextra  -pedantic -Wformat-security -Wfloat-equal -Wshadow -Wwrite-strings #-Wconversion
OPTIMISATIONS= -O2  -march=native
LDLIBS=$(shell pkg-config --libs  zlib)
DEBUG= -O0 -g3 -ggdb -static-libasan  -fsanitize=address
DEBUG_VALGRIND= -O0 -g3 -ggdb -static-libasan
DEBUG_GPROF= -O0 -g3 -ggdb

.PHONY : test

gdcv: gdcv.c utils.c dictzip.c utf8proc/utf8proc.c stardict.c dictd.c dsl.c dictionaries.c index.c cvec.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(OPTIMISATIONS)

gdcv_debug: gdcv.c  utils.c dictzip.c utf8proc/utf8proc.c  stardict.c dictd.c dsl.c dictionaries.c index.c cvec.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG)

gdcv_valgrind: gdcv.c  utils.c dictzip.c utf8proc/utf8proc.c  stardict.c dictd.c dsl.c dictionaries.c index.c cvec.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG_VALGRIND)
#	G_SLICE=always-malloc G_DEBUG=gc-friendly  valgrind -v --tool=memcheck --leak-check=full --num-callers=40 --log-file=valgrind.log --track-origins=yes ./$@ ../Dicts

gdcv_gprof: gdcv.c  utils.c dictzip.c utf8proc/utf8proc.c  stardict.c dictd.c dsl.c dictionaries.c index.c cvec.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG_GRPROF)
#	./$@ ../Dicts
#	gprof  $@ gmon.out > prof_output

clean:
	rm -f  gdcv  gdcv_debug gdcv_valgrind gdcv_gprof
