#! /bin/sh

libtoolize --copy --force
aclocal
autoheader
automake --gnu --add-missing --copy
autoconf
