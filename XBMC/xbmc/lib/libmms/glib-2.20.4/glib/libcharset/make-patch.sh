#!/bin/sh

if test $# = 1 ; then 
  ORIGINAL=$1
else
  echo "Usage: make-patch.sh /path/to/libcharset" 1>&2
  exit 1
fi

if test -f $ORIGINAL/lib/localcharset.c ; then : ; else
  echo "Usage: make-patch.sh /path/to/libcharset" 1>&2
  exit 1
fi

VERSION=`grep VERSION= $ORIGINAL/configure.ac | sed s/VERSION=//`

echo "# Patch against libcharset version $VERSION" > libcharset-glib.patch

for i in localcharset.c ref-add.sin ref-del.sin ; do
  diff -u $ORIGINAL/lib/$i $i >> libcharset-glib.patch
done

for i in glibc21.m4 codeset.m4 ; do
  diff -u $ORIGINAL/m4/$i $i >> libcharset-glib.patch
done

diff -u $ORIGINAL/include/libcharset.h.in libcharset.h >> libcharset-glib.patch
diff -u $ORIGINAL/include/localcharset.h.in localcharset.h >> libcharset-glib.patch
