import unittest
import argparse
import os
import sys

from spec_tests import get_tests

here = os.path.abspath(os.path.dirname(__file__))
sys.path.append(here)
sys.path.append(os.path.join(here, os.pardir, 'src'))
sys.path.append(os.path.join(here, os.pardir, 'wrappers'))

from remarkor import *

if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('libdir')
    parser.add_argument('specpath')
    args = parser.parse_known_args()
    conf.set_library_path(args[0].libdir)
    SPEC_PATH = args[0].specpath

class TestRemarkorMeta(type):
    def __new__(mcs, name, bases, dict):
        def gen_test(test_description):
            def test(self):
                remarkor = Remarkor(test_description['markdown'])
                remarkor.remark(width=1, validate=True)
            return test

        for t in get_tests(SPEC_PATH):
            test_name = 'test_%s' % re.sub('\W|^(?=\d)','_', t['section'])
            cnt = 1
            while '%s_%d' % (test_name, cnt) in dict:
                cnt += 1
            test_name = '%s_%d' % (test_name, cnt)
            dict[test_name] = gen_test(t)
        return type.__new__(mcs, name, bases, dict)

class TestRemarkor(unittest.TestCase, metaclass=TestRemarkorMeta):
    pass

if __name__=='__main__':
    unittest.main(argv=[sys.argv[0]] + args[1])
