#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import argparse
import sys
import platform
import itertools
import multiprocessing
import time
from cmark import CMark

TIMEOUT = 10

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
    COUNT = 50000

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

    return document, re.compile("(<p>\[%s\]</p>\n){%d}" % (bad_key, COUNT-1))


# list of pairs consisting of input and a regex that must match the output.
pathological = {
    # note - some pythons have limit of 65535 for {num-matches} in re.
    "nested strong emph":
                (("*a **a " * 65000) + "b" + (" a** a*" * 65000),
                 re.compile("(<em>a <strong>a ){65000}b( a</strong> a</em>){65000}")),
    "many emph closers with no openers":
                 (("a_ " * 65000),
                  re.compile("(a[_] ){64999}a_")),
    "many emph openers with no closers":
                 (("_a " * 65000),
                  re.compile("(_a ){64999}_a")),
    "many link closers with no openers":
                 (("a]" * 65000),
                  re.compile("(a\]){65000}")),
    "many link openers with no closers":
                 (("[a" * 65000),
                  re.compile("(\[a){65000}")),
    "mismatched openers and closers":
                 (("*a_ " * 50000),
                  re.compile("([*]a[_] ){49999}[*]a_")),
    "openers and closers multiple of 3":
                 (("a**b" + ("c* " * 50000)),
                  re.compile("a[*][*]b(c[*] ){49999}c[*]")),
    "link openers and emph closers":
                 (("[ a_" * 50000),
                  re.compile("(\[ a_){50000}")),
    "pattern [ (]( repeated":
                 (("[ (](" * 80000),
                  re.compile("(\[ \(\]\(){80000}")),
    "hard link/emph case":
                 ("**x [a*b**c*](d)",
                  re.compile("\\*\\*x <a href=\"d\">a<em>b\\*\\*c</em></a>")),
    "nested brackets":
                 (("[" * 50000) + "a" + ("]" * 50000),
                  re.compile("\[{50000}a\]{50000}")),
    "nested block quotes":
                 ((("> " * 50000) + "a"),
                  re.compile("(<blockquote>\n){50000}")),
    "deeply nested lists":
                 ("".join(map(lambda x: ("  " * x + "* a\n"), range(0,1000))),
                  re.compile("<ul>\n(<li>a\n<ul>\n){999}<li>a</li>\n</ul>\n(</li>\n</ul>\n){999}")),
    "U+0000 in input":
                 ("abc\u0000de\u0000",
                  re.compile("abc\ufffd?de\ufffd?")),
    "backticks":
                 ("".join(map(lambda x: ("e" + "`" * x), range(1,5000))),
                  re.compile("^<p>[e`]*</p>\n$")),
    "unclosed links A":
                 ("[a](<b" * 30000,
                  re.compile("(\[a\]\(&lt;b){30000}")),
    "unclosed links B":
                 ("[a](b" * 30000,
                  re.compile("(\[a\]\(b){30000}")),
    "reference collisions": hash_collisions()
#    "many references":
#                 ("".join(map(lambda x: ("[" + str(x) + "]: u\n"), range(1,5000 * 16))) + "[0] " * 5000,
#                  re.compile("(\[0\] ){4999}"))
    }

whitespace_re = re.compile('/s+/')

results = {'passed': [], 'errored': [], 'failed': [], 'ignored': []}

def run_pathological_test(description, results):
    (inp, regex) = pathological[description]
    [rc, actual, err] = cmark.to_html(inp)
    extra = ""
    if rc != 0:
        print(description, '[ERRORED (return code %d)]' %rc)
        print(err)
        if allowed_failures[description]:
            results['ignored'].append(description)
        else:
            results['errored'].append(description)
    elif regex.search(actual):
        print(description, '[PASSED]')
        results['passed'].append(description)
    else:
        print(description, '[FAILED]')
        print(repr(actual))
        if allowed_failures[description]:
            results['ignored'].append(description)
        else:
            results['failed'].append(description)

def run_tests():
    print("Testing pathological cases:")
    for description in pathological:
        p = multiprocessing.Process(target=run_pathological_test,
                  args=(description, results,))
        p.start()
        # wait TIMEOUT seconds or until it finishes
        p.join(TIMEOUT)
        # kill it if still active
        if p.is_alive():
            print(description, '[TIMEOUT]')
            if allowed_failures[description]:
                results['ignored'].append(description)
            else:
                results['errored'].append(description)
            p.terminate()
            p.join()

    passed  = len(results['passed'])
    failed  = len(results['failed'])
    errored = len(results['errored'])
    ignored = len(results['ignored'])

    print("%d passed, %d failed, %d errored" % (passed, failed, errored))
    if ignored > 0:
        print("Ignoring these allowed failures:")
        for x in results['ignored']:
            print(x)
    if failed == 0 and errored == 0:
        exit(0)
    else:
        exit(1)

if __name__ == "__main__":
    run_tests()
