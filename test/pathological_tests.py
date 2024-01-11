#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import argparse
import sys
import platform
import itertools
import multiprocessing
import queue
import time
from cmark import CMark

TIMEOUT = 5

parser = argparse.ArgumentParser(description='Run cmark tests.')
parser.add_argument('--program', dest='program', nargs='?', default=None,
        help='program to test')
parser.add_argument('--library-dir', dest='library_dir', nargs='?',
        default=None, help='directory containing dynamic library')
args = parser.parse_args(sys.argv[1:])

allowed_failures = {"many references": True}

cmark = CMark(prog=args.program, library_dir=args.library_dir)

def hash_collisions():
    REFMAP_SIZE = 16
    COUNT = 25000

    def badhash(ref):
        h = 0
        for c in ref:
            a = (h << 6) & 0xFFFFFFFF
            b = (h << 16) & 0xFFFFFFFF
            h = ord(c) + a + b - h
            h = h & 0xFFFFFFFF

        return (h % REFMAP_SIZE) == 0

    keys = ("x%d" % i for i in itertools.count())
    collisions = itertools.islice((k for k in keys if badhash(k)), COUNT)
    bad_key = next(collisions)

    document = ''.join("[%s]: /url\n\n[%s]\n\n" % (key, bad_key) for key in collisions)

    return document, re.compile(r"(<p>\[%s]</p>\n){%d}" % (bad_key, COUNT-1))


# list of pairs consisting of input and a regex that must match the output.
pathological = {
    # note - some pythons have limit of 65535 for {num-matches} in re.
    "nested strong emph":
                (("*a **a " * 32500) + "b" + (" a** a*" * 32500),
                 re.compile("(<em>a <strong>a ){32500}b( a</strong> a</em>){32500}")),
    "many emph closers with no openers":
                 (("a_ " * 32500),
                  re.compile("(a[_] ){32499}a_")),
    "many emph openers with no closers":
                 (("_a " * 32500),
                  re.compile("(_a ){32499}_a")),
    "many link closers with no openers":
                 (("a]" * 32500),
                  re.compile("(a\\]){32500}")),
    "many link openers with no closers":
                 (("[a" * 32500),
                  re.compile("(\\[a){32500}")),
    "mismatched openers and closers":
                 (("*a_ " * 25000),
                  re.compile("([*]a[_] ){24999}[*]a_")),
    "issue #389":
                 (("*a " * 20000 + "_a*_ " * 20000),
                  re.compile("(<em>a ){20000}(_a<\\/em>_ ?){20000}")),
    "openers and closers multiple of 3":
                 (("a**b" + ("c* " * 25000)),
                  re.compile("a[*][*]b(c[*] ){24999}c[*]")),
    "link openers and emph closers":
                 (("[ a_" * 25000),
                  re.compile("(\\[ a_){25000}")),
    "pattern [ (]( repeated":
                 (("[ (](" * 40000),
                  re.compile("(\\[ \\(\\]\\(){40000}")),
    "pattern ![[]() repeated":
                 ("![[]()" * 160000,
                  re.compile("(!\\[<a href=\"\"></a>){160000}")),
    "hard link/emph case":
                 ("**x [a*b**c*](d)",
                  re.compile("\\*\\*x <a href=\"d\">a<em>b\\*\\*c</em></a>")),
    "nested brackets":
                 (("[" * 25000) + "a" + ("]" * 25000),
                  re.compile("\\[{25000}a\\]{25000}")),
    "nested block quotes":
                 ((("> " * 25000) + "a"),
                  re.compile("(<blockquote>\n){25000}")),
    "deeply nested lists":
                 ("".join(map(lambda x: ("  " * x + "* a\n"), range(0,500))),
                  re.compile("<ul>\n(<li>a\n<ul>\n){499}<li>a</li>\n</ul>\n(</li>\n</ul>\n){499}")),
    "U+0000 in input":
                 ("abc\u0000de\u0000",
                  re.compile("abc\ufffd?de\ufffd?")),
    "backticks":
                 ("".join(map(lambda x: ("e" + "`" * x), range(1,2500))),
                  re.compile("^<p>[e`]*</p>\n$")),
    "unclosed links A":
                 ("[a](<b" * 30000,
                  re.compile(r"(\[a]\(&lt;b){30000}")),
    "unclosed links B":
                 ("[a](b" * 30000,
                  re.compile(r"(\[a]\(b){30000}")),
    "unclosed <!--":
                 ("</" + "<!--" * 300000,
                  re.compile("\\&lt;\\/(\\&lt;!--){300000}")),
    "empty lines in deeply nested lists":
                 ("- " * 30000 + "x" + "\n" * 30000,
                  re.compile(r"^(<\w+>\n?)+x(</\w+>\n)+$")),
    "empty lines in deeply nested lists in blockquote":
                 ("> " + "- " * 30000 + "x\n" + ">\n" * 30000,
                  re.compile(r"^(<\w+>\n?)+x(</\w+>\n)+$")),
    "emph in deep blockquote":
                 (">" * 100000 + "a*" * 100000,
                  re.compile(r"^(<\w+>\n)+<p>.*</p>\n(</\w+>\n)+$")),
    "reference collisions": hash_collisions()
#    "many references":
#                 ("".join(map(lambda x: ("[" + str(x) + "]: u\n"), range(1,5000 * 16))) + "[0] " * 5000,
#                  re.compile("(\\[0\\] ){4999}"))
    }

pathological_cmark = {
    "nested inlines":
                 ("*" * 20000 + "a" + "*" * 20000,
                  re.compile("^\\*+a\\*+$")),
    }

whitespace_re = re.compile('/s+/')

def run_pathological(q, inp):
    q.put(cmark.to_html(inp))

def run_pathological_cmark(q, inp):
    q.put(cmark.to_commonmark(inp))

def run_tests():
    q = multiprocessing.Queue()
    passed = []
    errored = []
    failed = []
    ignored = []

    print("Testing pathological cases:")
    for description in (*pathological, *pathological_cmark):
        if description in pathological:
            (inp, regex) = pathological[description]
            p = multiprocessing.Process(target=run_pathological,
                      args=(q, inp))
        else:
            (inp, regex) = pathological_cmark[description]
            p = multiprocessing.Process(target=run_pathological_cmark,
                      args=(q, inp))
        p.start()
        try:
            # wait TIMEOUT seconds or until it finishes
            rc, actual, err = q.get(True, TIMEOUT)
            p.join()
            if rc != 0:
                print(description, '[ERRORED (return code %d)]' %rc)
                print(err)
                if description in allowed_failures:
                    ignored.append(description)
                else:
                    errored.append(description)
            elif regex.search(actual):
                print(description, '[PASSED]')
                passed.append(description)
            else:
                print(description, '[FAILED]')
                print(repr(actual[:60]))
                if description in allowed_failures:
                    ignored.append(description)
                else:
                    failed.append(description)
        except queue.Empty:
            p.terminate()
            p.join()
            print(description, '[TIMEOUT]')
            if description in allowed_failures:
                ignored.append(description)
            else:
                errored.append(description)

    print("%d passed, %d failed, %d errored" %
          (len(passed), len(failed), len(errored)))
    if ignored:
        print("Ignoring these allowed failures:")
        for x in ignored:
            print(x)
    if failed or errored:
        exit(1)
    else:
        exit(0)

if __name__ == "__main__":
    run_tests()
