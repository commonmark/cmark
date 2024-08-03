#!/usr/bin/env python

# Creates a man page from a C file.

# Comments beginning with `/**` are treated as Groff man, except that
# 'this' is converted to \fIthis\f[], and ''this'' to \fBthis\f[].

# Non-blank lines immediately following a man page comment are treated
# as function signatures or examples and parsed into .Ft, .Fo, .Fa, .Fc. The
# immediately preceding man documentation chunk is printed after the example
# as a comment on it.

# That's about it!

import sys, re, os, platform
from datetime import date
from ctypes import CDLL, c_char_p, c_long, c_void_p

sysname = platform.system()

curdir = os.getcwd()

if sysname == 'Darwin':
    cmark = CDLL(curdir + "/build/src/libcmark.dylib")
else:
    cmark = CDLL(curdir + "/build/src/libcmark.so")

parse_document = cmark.cmark_parse_document
parse_document.restype = c_void_p
parse_document.argtypes = [c_char_p, c_long]

render_man = cmark.cmark_render_man
render_man.restype = c_char_p
render_man.argtypes = [c_void_p, c_long, c_long]

cmark_version_string = cmark.cmark_version_string
cmark_version_string.restype = c_char_p
cmark_version_string.argtypes = []

def md2man(text):
    if sys.version_info >= (3,0):
        textbytes = text.encode('utf-8')
        textlen = len(textbytes)
        return render_man(parse_document(textbytes, textlen), 0, 72).decode('utf-8')
    else:
        textbytes = text
        textlen = len(text)
        return render_man(parse_document(textbytes, textlen), 0, 72)

comment_start_re = re.compile(r'^\/\*\* ?')
comment_delim_re = re.compile(r'^[/ ]\** ?')
comment_end_re = re.compile(r'^ \**\/')
function_re = re.compile(r'^ *(?:CMARK_EXPORT\s+)?(?P<type>(?:const\s+)?\w+(?:\s*[*])?)\s*(?P<name>\w+)\s*\((?P<args>[^)]*)\)')
blank_re = re.compile(r'^\s*$')
macro_re = re.compile(r'CMARK_EXPORT *')
typedef_start_re = re.compile(r'typedef.*{$')
typedef_end_re = re.compile(r'}')
single_quote_re = re.compile(r"(?<!\w)'([^']+)'(?!\w)")
double_quote_re = re.compile(r"(?<!\w)''([^']+)''(?!\w)")

def handle_quotes(s):
    return re.sub(double_quote_re, r'**\g<1>**', re.sub(single_quote_re, r'*\g<1>*', s))

typedef = False
mdlines = []
chunk = []
sig = []

if len(sys.argv) > 1:
    sourcefile = sys.argv[1]
else:
    print("Usage:  make_man_page.py sourcefile")
    exit(1)

with open(sourcefile, 'r') as cmarkh:
    state = 'default'
    for line in cmarkh:
        # state transition
        oldstate = state
        if comment_start_re.match(line):
            state = 'man'
        elif comment_end_re.match(line) and state == 'man':
            continue
        elif comment_delim_re.match(line) and state == 'man':
            state = 'man'
        elif not typedef and blank_re.match(line):
            state = 'default'
        elif typedef and typedef_end_re.match(line):
            typedef = False
        elif typedef_start_re.match(line):
            typedef = True
            state = 'signature'
        elif state == 'man':
            state = 'signature'

        # handle line
        if state == 'man':
            chunk.append(handle_quotes(re.sub(comment_delim_re, '', line)))
        elif state == 'signature':
            ln = re.sub(macro_re, '', line)
            if typedef or not re.match(blank_re, ln):
                sig.append(ln)
        elif oldstate == 'signature' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            rawsig = ''.join(sig)
            m = function_re.match(rawsig)
            mdlines.append('.PP\n')
            if m:
                mdlines.append('\\fI' + m.group('type') + '\\f[]' + ' ')
                mdlines.append('\\fB' + m.group('name') + '\\f[]' + '(')
                first = True
                for argument in re.split(',', m.group('args')):
                    if not first:
                        mdlines.append(', ')
                    first = False
                    mdlines.append('\\fI' + argument.strip() + '\\f[]')
                mdlines.append(')\n')
            else:
                mdlines.append('.nf\n\\fC\n.RS 0n\n')
                mdlines += sig
                mdlines.append('.RE\n\\f[]\n.fi\n')
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += md2man(''.join(chunk))
            mdlines.append('\n')
            chunk = []
            sig = []
        elif oldstate == 'man' and state != 'signature':
            if len(mdlines) > 0 and mdlines[-1] != '\n':
                mdlines.append('\n')
            mdlines += md2man(''.join(chunk)) # add man chunk
            chunk = []
            mdlines.append('\n')

sys.stdout.write('.TH ' + os.path.basename(sourcefile).replace('.h','') + ' 3 "' + date.today().strftime('%B %d, %Y') + '" "cmark ' + cmark_version_string().decode('utf-8') + '" "Library Functions Manual"\n')
sys.stdout.write(''.join(mdlines))
