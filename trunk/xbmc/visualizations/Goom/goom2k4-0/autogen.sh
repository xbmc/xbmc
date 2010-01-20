#!/bin/sh

if libtoolize --version 2> /dev/null; then
aclocal -I m4 && libtoolize --copy --force && automake --foreign --add-missing && autoconf && ./configure $*
else
aclocal -I m4 && glibtoolize --copy --force && automake --foreign --add-missing && autoconf && ./configure $*
fi

