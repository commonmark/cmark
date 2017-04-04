FROM debian:jessie

RUN apt-get update && apt-get install -y \
  build-essential autoconf libtool \
  git \
  pkg-config \
  && apt-get clean

RUN apt-get install -y \
  cmake \
  gdb \
  valgrind \
  python3

RUN apt-get install -y \
  wget \
  clang

RUN wget http://lcamtuf.coredump.cx/afl/releases/afl-latest.tgz && \
	tar xf afl-latest.tgz && \
	cd afl-* && \
	make install && \
	cd .. && \
	rm -rf afl-*

RUN apt-get install -y man

RUN wget https://github.com/skvadrik/re2c/releases/download/0.15.3/re2c-0.15.3.tar.gz && \
	tar xf re2c-0.15.3.tar.gz && \
	cd re2c-* && \
	./configure && \
	make install && \
	cd .. && \
	rm -rf re2c-*

RUN apt-get install -y clang-format-3.5
