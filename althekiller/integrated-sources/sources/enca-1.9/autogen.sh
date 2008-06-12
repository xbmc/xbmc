#!/bin/sh
# @(#) $Id: autogen.sh,v 1.9 2004/11/17 18:11:49 yeti Exp $
# Run this to generate all the initial makefiles, etc.
# Tweaked by David Necas (Yeti) <yeti@physics.muni.cz> from various other
# autogen.sh's.  This file is in public domain.

DIE=0

PROJECT=Enca
ACLOCAL_FLAGS="-I m4"

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**ERROR**: You must have \`autoconf' installed to re-generate"
  echo "all the $PROJECT Makefiles."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/."
  DIE=1
  NO_AUTOCONF=yes
}

(grep "^AM_PROG_LIBTOOL" ./configure.ac >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.4.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
    NO_LIBTOOL=yes
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**ERROR**: You must have \`automake' installed to re-generate"
  echo "all the $PROJECT Makefiles."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.8.3.tar.gz"
  echo "(or a newer version if it is available) and read README.devel."
  DIE=1
  NO_AUTOMAKE=yes
}

# The world is curel.
if test -z "$NO_AUTOCONF"; then
  AC_VERSION=`autoconf --version | sed -e '2,$ d' -e 's/ *([^()]*)$//' -e 's/.* \(.*\)/\1/' -e 's/-p[0-9]\+//'`
  if test "$AC_VERSION" '<' "2.52"; then
    echo
    echo "**ERROR**: You need at least autoconf-2.52 installed to re-generate"
    echo "all the $PROJECT Makefiles."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/."
    DIE=1
  else
    echo "Autoconf $AC_VERSION: OK"
  fi
fi

if test -z "$NO_AUTOMAKE"; then
  AM_VERSION=`automake --version | sed -e '2,$ d' -e 's/ *([^()]*)$//' -e 's/.* \(.*\)/\1/' -e 's/-p[0-9]\+//'`
  if test "$AM_VERSION" '<' "1.8"; then
    echo
    echo "**ERROR**: You need at least automake-1.8 installed to re-generate"
    echo "all the $PROJECT Makefiles."
    echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.8.3.tar.gz"
    echo "(or a newer version if it is available) and read README.devel."
    DIE=1
  else
    echo "Automake $AM_VERSION: OK"
  fi
fi

# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**ERROR**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.8.3.tar.gz"
  echo "(or a newer version if it is available) and read README.devel."
  DIE=1
}

if test -z "$NO_LIBTOOL"; then
  LT_VERSION=`libtool --version | sed -e '2,$ d' -e 's/ *([^()]*)$//' -e 's/.* \(.*\)/\1/' -e 's/-p[0-9]\+//'`
  if test "$LT_VERSION" '<' "1.4"; then
    echo
    echo "**ERROR**: You need at least libtool-1.4 installed to re-generate"
    echo "all the $PROJECT Makefiles."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.4.tar.gz"
    echo "(or a newer version if it is available) and read README.devel."
    DIE=1
  else
    echo "Libtool $LT_VERSION: OK"
  fi
fi

if test "$DIE" -eq 1; then
  exit 1
fi

case $CC in
*xlc | *xlc\ * | *lcc | *lcc\ * )
  am_opt=--include-deps;;
esac

dir=.
echo processing $dir
(cd $dir && \
  libtoolize --force --copy && \
  aclocal $ACLOCAL_FLAGS && \
  autoheader && \
  automake --add-missing $am_opt && \
  autoconf) || {
    echo "**ERROR**: Re-generating failed.  You are allowed to shoot $PROJECT maintainer."
    echo "(BTW, why are you re-generating everything? Have you read README.devel?)"
    exit 1
  }

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo "\`$0' command line."
  echo
fi

./configure "$@"
