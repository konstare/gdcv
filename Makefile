CC=gcc
#clang
CFLAGS=$(pkg-config --cflags  zlib) -Wall  -Wextra  -pedantic
OPTIMISATIONS= -O2  -march=native
LDLIBS=$(shell pkg-config --libs  zlib)
DEBUG=-fsanitize=address -fno-omit-frame-pointer -ggdb -static-libasan 

.PHONY : test

gdcv-elisp.so: elisp.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c
	$(CC) -shared -o $@ -fPIC $^ $(CFLAGS) $(LDLIBS)   $(OPTIMISATIONS)

gdcv: gdcv.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(OPTIMISATIONS)

gdcv_debug: gdcv.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG)

clean:
	rm -f  gdcv gdcv-elisp.so gdcv_debug

