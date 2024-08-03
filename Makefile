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
BENCHSAMPLES=$(wildcard $(BENCHDIR)/samples/*.md)
BENCHFILE=$(BENCHDIR)/benchinput.md
ALLTESTS=alltests.md
NUMRUNS?=10
CMARK=$(BUILDDIR)/src/cmark
CMARK_FUZZ=$(BUILDDIR)/src/cmark-fuzz
PROG?=$(CMARK)
VERSION?=$(SPECVERSION)
RELEASE?=cmark-$(VERSION)
INSTALL_PREFIX?=/usr/local
CLANG_CHECK?=clang-check
CLANG_FORMAT=clang-format -style llvm -sort-includes=0 -i
AFL_PATH?=/usr/local/bin

.PHONY: all cmake_build leakcheck clean fuzztest test debug ubsan asan mingw archive newbench bench format update-spec afl libFuzzer lint

all: cmake_build man/man3/cmark.3

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
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		-DBUILD_SHARED_LIBS=YES

install: $(BUILDDIR)
	$(MAKE) -C $(BUILDDIR) install

uninstall: $(BUILDDIR)/install_manifest.txt
	xargs rm < $<

debug:
	mkdir -p $(BUILDDIR); \
	cd $(BUILDDIR); \
	cmake .. \
		-DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_SHARED_LIBS=YES; \
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
	cd $(BUILDDIR) && cmake .. -DBUILD_TESTING=NO -DCMAKE_C_COMPILER=$(AFL_PATH)/afl-clang
	$(MAKE)
	$(AFL_PATH)/afl-fuzz \
	    -i fuzz/afl_test_cases \
	    -o fuzz/afl_results \
	    -x fuzz/dictionary \
	    -t 100 \
	    $(CMARK) $(CMARK_OPTS)

libFuzzer:
	cmake \
	    -S . -B $(BUILDDIR) \
	    -DCMAKE_C_COMPILER=clang \
	    -DCMAKE_CXX_COMPILER=clang++ \
	    -DCMAKE_BUILD_TYPE=Asan \
	    -DCMARK_LIB_FUZZER=ON
	cmake --build $(BUILDDIR)
	mkdir -p fuzz/corpus
	$(BUILDDIR)/fuzz/cmark-fuzz \
	    -dict=fuzz/dictionary \
	    -max_len=1000 \
	    -timeout=1 \
	    fuzz/corpus

lint: $(BUILDDIR)
	errs=0 ; \
	for f in `ls src/*.[ch] | grep -v "scanners.c"` ; \
	  do echo $$f ; clang-tidy -header-filter='^build/.*' -p=build -warnings-as-errors='*' $$f || errs=1 ; done ; \
	exit $$errs

mingw:
	mkdir -p $(MINGW_BUILDDIR); \
	cd $(MINGW_BUILDDIR); \
	cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$(MINGW_INSTALLDIR) ;\
	$(MAKE) && $(MAKE) install

man/man3/cmark.3: src/cmark.h | $(CMARK)
	python3 man/make_man_page.py $< > $@ \

archive:
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).tar.gz HEAD
	git archive --prefix=$(RELEASE)/ -o $(RELEASE).zip HEAD

clean:
	rm -rf $(BUILDDIR) $(MINGW_BUILDDIR) $(MINGW_INSTALLDIR)

# We include case_fold.inc in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/case_fold.inc: $(DATADIR)/CaseFolding.txt
	python3 tools/make_case_fold_inc.py < $< > $@

# We include scanners.c in the repository, so this shouldn't
# normally need to be generated.
$(SRCDIR)/scanners.c: $(SRCDIR)/scanners.re
	@case "$$(re2c -v)" in \
	    *\ 0.13.*|*\ 0.14|*\ 0.14.1) \
		echo "re2c >= 0.14.2 is required"; \
		false; \
		;; \
	esac
	re2c -W -Werror --case-insensitive -b -i --no-generation-date \
		-o $@ $<
	$(CLANG_FORMAT) $@

# We include entities.inc in the repository, so normally this
# doesn't need to be regenerated:
$(SRCDIR)/entities.inc: tools/make_entities_inc.py
	python3 $< > $@

update-spec:
	curl 'https://raw.githubusercontent.com/jgm/CommonMark/master/spec.txt'\
 > $(SPEC)

test: $(SPEC) cmake_build
	ctest --test-dir $(BUILDDIR) --output-on-failure || (cat $(BUILDDIR)/Testing/Temporary/LastTest.log && exit 1)

$(ALLTESTS): $(SPEC)
	python3 test/spec_tests.py --spec $< --dump-tests | python3 -c 'import json; import sys; tests = json.loads(sys.stdin.read()); print("\n".join([test["markdown"] for test in tests]))' > $@

leakcheck: $(ALLTESTS)
	for format in html man xml latex commonmark; do \
	  for opts in "" "--smart"; do \
	     echo "cmark -t $$format $$opts" ; \
	     valgrind -q --leak-check=full --dsymutil=yes --error-exitcode=1 $(PROG) -t $$format $$opts $(ALLTESTS) >/dev/null || exit 1;\
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
	cat progit/$$lang/*/*.markdown >> $@; \
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

operf: $(CMARK)
	operf $< < $(BENCHFILE) > /dev/null

distclean: clean
	-rm -rf *.dSYM
	-rm -f README.html
	-rm -rf $(BENCHFILE) $(ALLTESTS) progit
