#!/bin/sh

aclocal && glibtoolize -c -f && autoheader && automake -a -c -f && autoconf

