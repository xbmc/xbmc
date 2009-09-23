#!/bin/sh
#
# $Id: autogen.sh 1091 2008-06-08 06:37:22Z nicodvb $
#
# run this to generate all the initial makefiles, etc.

srcdir=`dirname "$0"`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd "$srcdir"

AUTORECONF=${AUTORECONF-autoreconf}

if ! type $AUTORECONF >/dev/null 2>&1; then
  echo "**Error**: Missing \`autoreconf' program." >&2
  echo "You will need the autoconf and automake packages." >&2
  echo "You can download them from ftp://ftp.gnu.org/pub/gnu/." >&2
  exit 1
fi

$AUTORECONF -v --install || exit $?
cd "$ORIGDIR" || exit $?

test "$1" = noconfig && NOCONFIGURE=1

if test -z "$NOCONFIGURE"; then
  "$srcdir"/configure "$@"
fi
