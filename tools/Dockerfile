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
