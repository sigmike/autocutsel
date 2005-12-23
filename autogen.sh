#!/bin/sh

set -e

rm -rf Makefile.in aclocal.m4 autom4te.cache config.h.in configure config.guess config.sub stamp-h.in

aclocal
libtoolize --force
autoheader
automake --add-missing
autoconf

