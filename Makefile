SRCDIR=src
EXTDIR=extensions
DATADIR=data
BUILDDIR?=build
GENERATOR?=Unix Makefiles
MINGW_BUILDDIR?=build-mingw
MINGW_INSTALLDIR?=windows
SPEC=test/spec.txt
EXTENSIONS_SPEC=test/extensions.txt
SITE=_site
SPECVERSION=$(shell perl -ne 'print $$1 if /^version: *([0-9.]+)/' $(SPEC))
FUZZCHARS?=2000000  # for fuzztest
BENCHDIR=bench
BENCHSAMPLES=$(wildcard $(BENCHDIR)/samples/*.md)
BENCHFILE=$(BENCHDIR)/benchinput.md
ALLTESTS=alltests.md
NUMRUNS?=20
CMARK=$(BUILDDIR)/src/cmark-gfm
CMARK_FUZZ=$(BUILDDIR)/src/cmark-fuzz
PROG?=$(CMARK)
VERSION?=$(SPECVERSION)
RELEASE?=CommonMark-$(VERSION)
INSTALL_PREFIX?=/usr/local
CLANG_CHECK?=clang-check
CLANG_FORMAT=clang-format -style llvm -sort-includes=0 -i
AFL_PATH?=/usr/local/bin

.PHONY: all cmake_build leakcheck clean fuzztest test debug ubsan asan mingw archive newbench bench format update-spec afl clang-check docker libFuzzer

all: cmake_build man/man3/cmark-gfm.3

$(CMARK): cmake_build

cmake_build: $(BUILDDIR)
	@$(MAKE) -j2 -C $(BUILDDIR)
	@echo "Binaries can be found in $(BUILDDIR)/src"

$(BUILDDIR):
	@cmake --version > /dev/null || (echo "You need cmake to build this program: http://www.cmake.org/download/" && exit 1)
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. \
		-G "$(GENERATOR)" \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(INSTALL_PREFIX) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

install: $(BUILDDIR)
	$(MAKE) -C $(BUILDDIR) install

debug:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Debug; \
	$(MAKE)

ubsan:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Ubsan; \
	$(MAKE)

asan:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Asan; \
	$(MAKE)

prof:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. -DCMAKE_BUILD_TYPE=Profile; \
	$(MAKE)

afl:
	@[ -n "$(AFL_PATH)" ] || { echo '$$AFL_PATH not set'; false; }
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake .. -DCMARK_TESTS=0 -DCMAKE_C_COMPILER=$(AFL_PATH)/afl-clang
	$(MAKE)
	$(AFL_PATH)/afl-fuzz \
	    -i test/afl_test_cases \
	    -o test/afl_results \
	    -x test/fuzzing_dictionary \
	    $(AFL_OPTIONS) \
	    -t 100 \
	    $(CMARK) -e table -e strikethrough -e autolink -e tagfilter $(CMARK_OPTS)

libFuzzer:
	@[ -n "$(LIB_FUZZER_PATH)" ] || { echo '$$LIB_FUZZER_PATH not set'; false; }
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=Asan -DCMARK_LIB_FUZZER=ON -DCMAKE_LIB_FUZZER_PATH=$(LIB_FUZZER_PATH) ..
	$(MAKE) -j2 -C $(BUILDDIR) cmark-fuzz
	test/run-cmark-fuzz $(CMARK_FUZZ)

clang-check: all
	${CLANG_CHECK} -p build -analyze src/*.c

mingw:
	mkdir -p $(MINGW_BUILDDIR); \
	cd $(MINGW_BUILDDIR); \
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$(MINGW_INSTALLDIR) ;\
	$(MAKE) && $(MAKE) install

man/man3/cmark-gfm.3: src/cmark-gfm.h | $(CMARK)
	python man/make_man_page.py $< > $@ \

archive:
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).tar.gz HEAD
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).zip HEAD

clean:
	rm -rf $(BUILDDIR) $(MINGW_BUILDDIR) $(MINGW_INSTALLDIR)

# We include case_fold_switch.inc in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/case_fold_switch.inc: $(DATADIR)/CaseFolding.txt
	perl tools/mkcasefold.pl < $< > $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	@case "$$(re2c -v)" in \
	    *\ 0.13.*|*\ 0.14|*\ 0.14.1) \
		echo "re2c >= 0.14.2 is required"; \
		false; \
		;; \
	esac
	re2c -W -Werror --case-insensitive -b -i --no-generation-date -8 \
		--encoding-policy substitute -o $@ $<
	$(CLANG_FORMAT) $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(EXTDIR)/ext_scanners.c: $(EXTDIR)/ext_scanners.re
	@case "$$(re2c -v)" in \
	    *\ 0.13.*|*\ 0.14|*\ 0.14.1) \
		echo "re2c >= 0.14.2 is required"; \
		false; \
		;; \
	esac
	re2c --case-insensitive -b -i --no-generation-date -8 \
		--encoding-policy substitute -o $@ $<
	clang-format -style llvm -i $@

# We include entities.inc in the repository, so normally this
# doesn't need to be regenerated:
$(SRCDIR)/entities.inc: tools/make_entities_inc.py
	python3 $< > $@

update-spec:
	curl 'https://raw.githubusercontent.com/jgm/CommonMark/master/spec.txt'\
 > $(SPEC)

test: $(SPEC) cmake_build
	$(MAKE) -C $(BUILDDIR) test || (cat $(BUILDDIR)/Testing/Temporary/LastTest.log && exit 1)

$(ALLTESTS): $(SPEC) $(EXTENSIONS_SPEC)
	( \
	  python3 test/spec_tests.py --spec $(SPEC) --dump-tests | \
	    python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); u8s = open(1, "w", encoding="utf-8", closefd=False); print("\n".join([test["markdown"] for test in tests]), file=u8s)'; \
	  python3 test/spec_tests.py --spec $(EXTENSIONS_SPEC) --dump-tests | \
	    python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); u8s = open(1, "w", encoding="utf-8", closefd=False); print("\n".join([test["markdown"] for test in tests]), file=u8s)'; \
	) > $@

leakcheck: $(ALLTESTS)
	for format in html man xml latex commonmark; do \
	  for opts in "" "--smart"; do \
	     echo "cmark-gfm -t $$format -e table -e strikethrough -e autolink -e tagfilter $$opts" ; \
	     valgrind -q --leak-check=full --dsymutil=yes --suppressions=suppressions --error-exitcode=1 $(PROG) -t $$format -e table -e strikethrough -e autolink -e tagfilter $$opts $(ALLTESTS) >/dev/null || exit 1;\
          done; \
	done;

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
		for i in `seq 1 10`; do \
			cat progit/$$lang/*/*.markdown >> $@; \
		done; \
	done

# for more accurate results, run with
# sudo renice -10 $$; make bench
bench: $(BENCHFILE)
	{ for x in `seq 1 $(NUMRUNS)` ; do \
		/usr/bin/env time -p $(PROG) </dev/null >/dev/null ; \
		/usr/bin/env time -p $(PROG) $< >/dev/null ; \
		done \
	} 2>&1  | grep 'real' | awk '{print $$2}' | python3 'bench/stats.py'

newbench:
	for f in $(BENCHSAMPLES) ; do \
	  printf "%26s  " `basename $$f` ; \
	  { for x in `seq 1 $(NUMRUNS)` ; do \
		/usr/bin/env time -p $(PROG) </dev/null >/dev/null ; \
		for x in `seq 1 200` ; do cat $$f ; done | \
		  /usr/bin/env time -p $(PROG) > /dev/null; \
		done \
	  } 2>&1  | grep 'real' | awk '{print $$2}' | \
	    python3 'bench/stats.py'; done

format:
	$(CLANG_FORMAT) src/*.c src/*.h api_test/*.c api_test/*.h

format-extensions:
	clang-format -style llvm -i extensions/*.c extensions/*.h

operf: $(CMARK)
	operf $< < $(BENCHFILE) > /dev/null

distclean: clean
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -rf $(BENCHFILE) $(ALLTESTS) progit

docker:
	docker build -t cmark-gfm $(CURDIR)/tools
	docker run --privileged -t -i -v $(CURDIR):/src/cmark-gfm -w /src/cmark-gfm cmark-gfm /bin/bash
