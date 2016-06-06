#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ctypes import CDLL, c_char_p, c_size_t, c_int, c_void_p
from subprocess import *
import platform
import os

def pipe_through_prog(prog, text):
    p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
    [result, err] = p1.communicate(input=text.encode('utf-8'))
    return [p1.returncode, result.decode('utf-8'), err]

def to_html(lib, text):
    markdown = lib.cmark_markdown_to_html
    markdown.restype = c_char_p
    markdown.argtypes = [c_char_p, c_size_t, c_int]
    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    result = markdown(textbytes, textlen, 0).decode('utf-8')
    return [0, result, '']

def to_commonmark(lib, text):
    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    parse_document = lib.cmark_parse_document
    parse_document.restype = c_void_p
    parse_document.argtypes = [c_char_p, c_size_t, c_int]
    render_commonmark = lib.cmark_render_commonmark
    render_commonmark.restype = c_char_p
    render_commonmark.argtypes = [c_void_p, c_int, c_int]
    node = parse_document(textbytes, textlen, 0)
    result = render_commonmark(node, 0, 0).decode('utf-8')
    return [0, result, '']

class CMark:
    def __init__(self, prog=None, library_dir=None):
        self.prog = prog
        if prog:
            self.to_html = lambda x: pipe_through_prog(prog, x)
            self.to_commonmark = lambda x: pipe_through_prog(prog + ' -t commonmark', x)
        else:
            sysname = platform.system()
            if sysname == 'Darwin':
                libnames = [ "libcmark.dylib" ]
            elif sysname == 'Windows':
                libnames = [ "cmark.dll", "libcmark.dll" ]
            else:
                libnames = [ "libcmark.so" ]
            if not library_dir:
                library_dir = os.path.join("build", "src")
            for libname in libnames:
                candidate = os.path.join(library_dir, libname)
                if os.path.isfile(candidate):
                    libpath = candidate
                    break
            cmark = CDLL(libpath)
            self.to_html = lambda x: to_html(cmark, x)
            self.to_commonmark = lambda x: to_commonmark(cmark, x)

