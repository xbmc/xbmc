#! /bin/sh

# Copyright 2007 Johannes Lehtinen
# This shell script is free software; Johannes Lehtinen gives
# unlimited permission to copy, distribute and modify it.

set -e

# Check directory
basedir="`dirname "$0"`"
if ! test -f "$basedir"/cpfile/Makefile.am; then
    echo 'Run autogen.sh in the examples source directory.' 1>&2
    exit 1
fi

# Generate files in examples directory
cd "$basedir"
test -d auxliary || mkdir auxliary
libtoolize --automake -f
aclocal
autoconf
automake -a
