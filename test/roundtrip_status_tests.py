#!/usr/bin/env python3
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

parser = argparse.ArgumentParser(description="Check roundtrip driver exit statuses.")
parser.add_argument("--library-dir", required=True)
args = parser.parse_args()
LIBRARY_DIR = str(Path(args.library_dir).resolve())
DRIVER = str(Path(__file__).with_name("roundtrip_tests.py").resolve())


class RoundtripStatusTests(unittest.TestCase):
    def run_examples(self, examples):
        fence = "`" * 32
        text = "".join(fence + " example\n" + markdown + ".\n" + html + fence + "\n"
                       for markdown, html in examples)
        with tempfile.TemporaryDirectory() as directory:
            fixture = Path(directory) / "examples.txt"
            with fixture.open("w", encoding="utf-8", newline="\n") as stream:
                stream.write(text)
            return subprocess.run(
                [sys.executable, DRIVER, "--spec", str(fixture),
                 "--library-dir", LIBRARY_DIR],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                universal_newlines=True, timeout=30)

    def test_matching_example_succeeds(self):
        result = self.run_examples([("hello\n", "<p>hello</p>\n")])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_mismatched_example_fails(self):
        result = self.run_examples([("hello\n", "<p>wrong</p>\n")])
        self.assertIn("--- expected HTML", result.stdout)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)

    def test_many_failures_do_not_wrap_to_zero(self):
        result = self.run_examples([("hello\n", "<p>wrong</p>\n")] * 256)
        self.assertEqual(result.returncode, 1, result.stdout[-200:] + result.stderr)

    def test_matching_examples_do_not_count_as_failures(self):
        result = self.run_examples([("hello\n", "<p>hello</p>\n"),
                                    ("world\n", "<p>wrong</p>\n")])
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
