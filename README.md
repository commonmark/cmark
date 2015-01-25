CommonMark
==========

CommonMark is a rationalized version of Markdown syntax,
with a [spec][the spec] and BSD3-licensed reference
implementations in C and JavaScript.

[Try it now!](http://spec.commonmark.org/dingus.html)

For more information, see <http://commonmark.org>.

This repository contains the C reference implementation.
It provides a library with functions for parsing CommonMark
documents to an abstract syntax tree (AST), manipulating the AST,
and rendering the document to HTML or to an XML representation of the
AST.  It also provides a command-line program, `cmark`, for
parsing and rendering CommonMark documents.

The library is fast, on par with [sundown]:  see the [benchmarks].

[sundown]: https://github.com/vmg/sundown
[benchmarks]: benchmarks.md

Installing
----------

Building the C program (`cmark`) and shared library (`libcmark`)
requires [cmake].  If you modify `scanners.re`, then you will also
need [re2c], which is used to generate `scanners.c` from
`scanners.re`.  We have included a pre-generated `scanners.c` in
the repository to reduce build dependencies.

If you have GNU make, you can simply `make`, `make test`, and `make
install`.  This calls [cmake] to create a `Makefile` in the `build`
directory, then uses that `Makefile` to create the executable and
library.  The binaries can be found in `build/src`.  The default
installation prefix is `/usr/local`.  To change the installation
prefix, pass the `INSTALL_PREFIX` variable if you run `make` for the
first time: `make INSTALL_PREFIX=path`.

For a more portable method, you can use [cmake] manually. [cmake] knows
how to create build environments for many build systems.  For example,
on FreeBSD:

    mkdir build
    cd build
    cmake ..  # optionally: -DCMAKE_INSTALL_PREFIX=path
    make      # executable will be created as build/src/cmark
    make test
    make install

Or, to create Xcode project files on OSX:

    mkdir build
    cd build
    cmake -G Xcode ..
    open cmark.xcodeproj

The GNU Makefile also provides a few other targets for developers.
To run a benchmark:

    make bench

To run a "fuzz test" against ten long randomly generated inputs:

    make fuzztest

To run a test for memory leaks using `valgrind`:

    make leakcheck

To reformat source code using `astyle`:

    make astyle

To make a release tarball and zip archive:

    make archive

Installing (Windows)
--------------------

To compile with MSVC and NMAKE:

    nmake

You can cross-compile a Windows binary and dll on linux if you have the
`mingw32` compiler:

    make mingw

The binaries will be in `build-mingw/windows/bin`.

Usage
-----

Instructions for the use of the command line program and library can
be found in the man pages in the `man` subdirectory.

**A note on security:**
This library does not attempt to sanitize link attributes or
raw HTML.  If you use it in applications that accept
untrusted user input, you must run the output through an HTML
sanitizer to protect against
[XSS attacks](http://en.wikipedia.org/wiki/Cross-site_scripting).

Contributing
------------

There is a [forum for discussing
CommonMark](http://talk.commonmark.org); you should use it instead of
github issues for questions and possibly open-ended discussions.
Use the [github issue tracker](http://github.com/jgm/CommonMark/issues)
only for simple, clear, actionable issues.

Authors
-------

John MacFarlane wrote the original library and program.
The block parsing algorithm was worked out together with David
Greenspan. Vicent Marti optimized the C implementation for
performance, increasing its speed tenfold.  Kārlis Gaņģis helped
work out a better parsing algorithm for links and emphasis,
eliminating several worst-case performance issues.
Nick Wellnhofer contributed many improvements, including
most of the C library's API and its test harness.

[cmake]: http://www.cmake.org/download/
[re2c]: http://re2c.org
