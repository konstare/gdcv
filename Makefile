CC=gcc
#clang
EMACS = emacs
CFLAGS=$(pkg-config --cflags  zlib) -Wall  -Wextra  -pedantic
OPTIMISATIONS= -O2  -march=native
LDLIBS=$(shell pkg-config --libs  zlib)
DEBUG=-fsanitize=address -O0 -g3 -ggdb -static-libasan 

.PHONY : test

gdcv: gdcv.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c 
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(OPTIMISATIONS)

emacs-module: gdcv-elisp.so gdcv.elc

gdcv-elisp.so: elisp.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c
	$(CC) -shared -o $@ -fPIC $^ $(CFLAGS) $(LDLIBS)   $(OPTIMISATIONS)

gdcv_debug: gdcv.c dictzip.c utils.c format.c index.c utfproc/utf8proc.c 
	$(CC)  -o $@ $^ $(CFLAGS) $(LDLIBS) $(DEBUG)

clean:
	rm -f  gdcv gdcv-elisp.so gdcv_debug gdcv.elc

.SUFFIXES: .el .elc
.el.elc:
	$(EMACS) -batch -Q -L . -f batch-byte-compile $<
