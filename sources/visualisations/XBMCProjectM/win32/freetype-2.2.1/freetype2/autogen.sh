#!/bin/sh

# Copyright 2005, 2006 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

run ()
{
  echo "running \`$*'"
  eval $*

  if test $? != 0 ; then
    echo "error while running \`$*'"
    exit 1
  fi
}

if test ! -f ./builds/unix/configure.raw; then
  echo "You must be in the same directory as \`autogen.sh'."
  echo "Bootstrapping doesn't work if srcdir != builddir."
  exit 1
fi

# This sets freetype_major, freetype_minor, and freetype_patch.
eval `sed -nf version.sed include/freetype/freetype.h`

# We set freetype-patch to an empty value if it is zero.
if test "$freetype_patch" = ".0"; then
  freetype_patch=
fi

cd builds/unix

echo "generating \`configure.ac'"
sed -e "s;@VERSION@;$freetype_major$freetype_minor$freetype_patch;" \
    < configure.raw > configure.ac

run aclocal -I . --force
run libtoolize --force --copy
run autoconf --force

chmod +x mkinstalldirs
chmod +x install-sh

cd ../..

chmod +x ./configure

# EOF
