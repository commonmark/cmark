CFLAGS = -fPIC -O3 -I$(CBITS) -I.
SRC=../src
SWIG ?= swig

.PHONY: clean

$(SRC)/cmark_wrap.c: cmark.i $(SRC)/cmark.h
	$(SWIG) -o $@ -lua -I../src -DCMARK_EXPORT='' $<

clean:
	rm $(SRC)/cmark_wrap.c
