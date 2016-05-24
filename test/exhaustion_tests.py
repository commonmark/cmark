#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import sys
import os
import tempfile
from subprocess import *

DATA = bytes("""
Ph'nglui mglw'nafh Cthulhu R'lyeh wgah'nagl fhtagn. Ya r'luh hlirghor uaaah ah, nilgh'ri fhtagnnyth ilyaa gof'nn ilyaa nog zhro hupadgh yaor, ah hupadgh ph'zhro h'uh'e Yoggoth vulgtm uh'e hai mnahn'. Phlegeth fm'latgh lw'nafh Cthulhu kn'a nog 'bthnk ya ehye fm'latgh y-n'gha, y-r'luh 'fhalmaoth gnaiih ep hai ep nog lloig grah'n hafh'drn zhroyar, mnahn' uh'e nnnch' chtenff uln f'ooboshu orr'e k'yarnak hlirgh. H'vulgtm ng'bthnk n'ghft stell'bsna hai nnnkadishtu lloig nglui nilgh'ri ron, nagnaiih ronnyth phlegeth f'ep ooboshuor mg y-Hastur shagg, f'Chaugnar Faugn bug uaaah ehye kn'a geb orr'e lw'nafh. Ilyaa shtunggli uh'e naflhrii k'yarnak ya, nog k'yarnak wgah'n shagg nnnhlirgh, gof'nnyar r'luh Azathoth wgah'n.
""", 'UTF-8')

# CMark has a default maximum buffer growth size of 32mb,
# which means that the total size of the buffer cannot be
# larger than (32 + 16)mb in total, accounting for overgrowth
MAX_FILE_SIZE = 48.0 # mb

def exhaustion(prog):
    with tempfile.TemporaryFile() as tmp:
        p1 = Popen(prog.split(), stdout=tmp, stdin=PIPE)
        written, read = 0, 0

        for i in range(512 * 512):
            p1.stdin.write(DATA)
            written += len(DATA)

        p1.stdin.close()
        res = p1.wait()
        written = written / (1024.0 * 1024.0)
        read = tmp.tell() / (1024.0 * 1024.0)

        if res != 0:
            raise Exception("CMark did not exit properly")
        if written <= read:
            raise Exception("Output was not truncated (%fmb -> %fmb)" % (written, read))
        if read > MAX_FILE_SIZE:
            raise Exception("Output was not truncated at the expected range (%fmb)" % (read))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Run cmark exhaustion tests')
    parser.add_argument('--program', dest='program', nargs='?', default=None,
            help='program to test')
    args = parser.parse_args(sys.argv[1:])
    exhaustion(args.program)
