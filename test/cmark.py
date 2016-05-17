#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ctypes import CDLL, c_char_p, c_long
from subprocess import *
import platform
import os

def pipe_through_prog(prog, text):
    p1 = Popen(prog.split(), stdout=PIPE, stdin=PIPE, stderr=PIPE)
    [result, err] = p1.communicate(input=text.encode('utf-8'))
    return [p1.returncode, result.decode('utf-8'), err]

def use_library(lib, text):
    textbytes = text.encode('utf-8')
    textlen = len(textbytes)
    return [0, lib(textbytes, textlen, 0).decode('utf-8'), '']

class CMark:
    def __init__(self, prog=None, library_dir=None):
        self.prog = prog
        if prog:
            self.to_html = lambda x: pipe_through_prog(prog, x)
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
            markdown = cmark.cmark_markdown_to_html
            markdown.restype = c_char_p
            markdown.argtypes = [c_char_p, c_long]
            self.to_html = lambda x: use_library(markdown, x)
