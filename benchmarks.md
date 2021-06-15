# Benchmarks

Here are some benchmarks, run on a 2.3GHz 8-core i9 macbook pro.
The input text is a 1106 KB Markdown file built by concatenating
the Markdown sources of all the localizations of the first edition
of [*Pro Git*](https://github.com/progit/progit/tree/master/en) by
Scott Chacon.

|Implementation     |  Time (sec)|
|-------------------|-----------:|
| **commonmark.js** |    0.59    |
| **cmark**         |    0.12    |
| **md4c**          |    0.04    |

To run these benchmarks, use `make bench PROG=/path/to/program`.

`time` is used to measure execution speed.  The reported
time is the *difference* between the time to run the program
with the benchmark input and the time to run it with no input.
(This procedure ensures that implementations in dynamic languages are
not penalized by startup time.) A median of ten runs is taken.  The
process is reniced to a high priority so that the system doesn't
interrupt runs.
