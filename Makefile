SRCDIR=src
DATADIR=data
BUILDDIR?=build
GENERATOR?=Unix Makefiles
MINGW_BUILDDIR?=build-mingw
MINGW_INSTALLDIR?=windows
SPEC=test/spec.txt
SITE=_site
SPECVERSION=$(shell perl -ne 'print $$1 if /^version: *([0-9.]+)/' $(SPEC))
FUZZCHARS?=2000000  # for fuzztest
BENCHDIR=bench
BENCHFILE=$(BENCHDIR)/benchinput.md
ALLTESTS=alltests.md
NUMRUNS?=10
CMARK=$(BUILDDIR)/src/cmark
PROG?=$(CMARK)
BENCHINP?=README.md
VERSION?=$(SPECVERSION)
RELEASE?=CommonMark-$(VERSION)
INSTALL_PREFIX?=/usr/local

.PHONY: all cmake_build spec leakcheck clean fuzztest dingus upload test update-site upload-site debug asan mingw archive bench astyle update-spec

all: cmake_build man/man3/cmark.3

$(CMARK): cmake_build

cmake_build: $(BUILDDIR) $(SRCDIR)/html_unescape.h $(SRCDIR)/case_fold_switch.inc $(SRCDIR)/scanners.c
	@make -j2 -C $(BUILDDIR)
	@echo "Binaries can be found in $(BUILDDIR)/src"

$(BUILDDIR):
	@cmake --version > /dev/null || (echo "You need cmake to build this program: http://www.cmake.org/download/" && exit 1)
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX)

install: $(BUILDDIR)
	make -C $(BUILDDIR) install

debug:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Debug; \
	make

asan:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Asan; \
	make

prof:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Profile; \
	make

afl:
	@[ -n "$(AFL_PATH)" ] || { echo '$$AFL_PATH not set'; false; }
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake .. -DCMAKE_C_COMPILER=$(AFL_PATH)/afl-gcc
	make
	$(AFL_PATH)/afl-fuzz \
	    -i test/afl_test_cases \
	    -o test/afl_results \
	    -m none \
	    $(CMARK)

mingw:
	mkdir -p $(MINGW_BUILDDIR); \
	cd $(MINGW_BUILDDIR); \
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$(MINGW_INSTALLDIR) ;\
	make && make install

man/man3/cmark.3: src/cmark.h | $(CMARK)
	python man/make_man_page.py $< > $@ \

archive:
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).tar.gz HEAD
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).zip HEAD

clean:
	rm -rf $(BUILDDIR) $(MINGW_BUILDDIR) $(MINGW_INSTALLDIR)

# We include html_unescape.h in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/html_unescape.h: $(SRCDIR)/html_unescape.gperf
	gperf -L ANSI-C -I -t -N find_entity -H hash_entity -K entity -C -l \
		-F ',{0}' --null-strings -m5 -P -Q entity_pool $< > $@

# We include case_fold_switch.inc in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding-3.2.0.txt
	perl tools/mkcasefold.pl < $< > $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	@case "$$(re2c -v)" in \
	    *\ 0.13.7*|*\ 0.14*) \
		echo $$(re2c -v) is known to produce buggy code.; \
		echo Try version 0.13.6 or below.; \
		false; \
		;; \
	esac
	re2c --case-insensitive -b -i --no-generation-date -o $@ $<

update-spec:
	curl 'https://raw.githubusercontent.com/jgm/CommonMark/master/spec.txt'\
 > $(SPEC)

test: $(SPEC) cmake_build
	make -C $(BUILDDIR) test || (cat $(BUILDDIR)/Testing/Temporary/LastTest.log && exit 1)

$(ALLTESTS): $(SPEC)
	python3 test/spec_tests.py --spec $< --dump-tests | python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); print("\n".join([test["markdown"] for test in tests]))' > $@

leakcheck: $(ALLTESTS)
	for format in html man xml commonmark; do \
	  cat $< | valgrind --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) -t $$format >/dev/null; \
	done

fuzztest:
	{ for i in `seq 1 10`; do \
	  cat /dev/urandom | head -c $(FUZZCHARS) | iconv -f latin1 -t utf-8 | tee fuzz-$$i.txt | \
		/usr/bin/env time -p $(PROG) >/dev/null && rm fuzz-$$i.txt ; \
	done } 2>&1 | grep 'user\|abnormally'

progit:
	git clone https://github.com/progit/progit.git

$(BENCHFILE): progit
	echo "" > $@
	for lang in ar az be ca cs de en eo es es-ni fa fi fr hi hu id it ja ko mk nl no-nb pl pt-br ro ru sr th tr uk vi zh zh-tw; do \
		cat progit/$$lang/*/*.markdown >> $@; \
	done

bench: $(BENCHFILE)
	{ sudo renice 99 $$$$; \
	  for x in `seq 1 $(NUMRUNS)` ; do \
	  /usr/bin/env time -p $(PROG) </dev/null >/dev/null ; \
	  /usr/bin/env time -p $(PROG) $< >/dev/null ; \
		  done \
	} 2>&1  | grep 'real' | awk '{print $$2}' | python3 'bench/stats.py'

astyle:
	astyle --style=linux -t -p -r  'src/*.c' --exclude=scanners.c
	astyle --style=linux -t -p -r  'src/*.h' --exclude=html_unescape.h

operf: $(CMARK)
	operf $< < $(BENCHINP) > /dev/null

distclean: clean
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -rf $(BENCHFILE) $(ALLTESTS) progit
