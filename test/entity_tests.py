#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import argparse
import sys
import platform
import html
from cmark import CMark

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark tests.')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    parser.add_argument('--library-dir', dest='library_dir', nargs='?',
            default=None, help='directory containing dynamic library')
    args = parser.parse_args(sys.argv[1:])

cmark = CMark(prog=args.program, library_dir=args.library_dir)

entities5 = html.entities.html5
entities = sorted([(k[:-1], entities5[k]) for k in entities5.keys() if k[-1] == ';'])

passed = 0
errored = 0
failed = 0

exceptions = {
    'quot': '&quot;',
    'QUOT': '&quot;',

    # These are broken, but I'm not too worried about them.
    'nvlt': '&lt;⃒',
    'nvgt': '&gt;⃒',
}

print("Testing entities:")
for entity, utf8 in entities:
    [rc, actual, err] = cmark.to_html("&{};".format(entity))
    check = exceptions.get(entity, utf8)

    if rc != 0:
        errored += 1
        print(entity, '[ERRORED (return code {})]'.format(rc))
        print(err)
    elif check in actual:
        print(entity, '[PASSED]')
        passed += 1
    else:
        print(entity, '[FAILED]')
        print(repr(actual))
        failed += 1

print("{} passed, {} failed, {} errored".format(passed, failed, errored))
if failed == 0 and errored == 0:
    exit(0)
else:
    exit(1)
