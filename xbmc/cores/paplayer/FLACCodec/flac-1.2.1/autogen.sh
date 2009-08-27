#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# (based on the version in enlightenment's cvs)

# Some notes:
#
# You may need to specify -I /SOME_PATH/share/aclocal in ACLOCAL_FLAGS
# if any packages FLAC relies on (autotools, libogg, libiconv) are
# installed in non-standard places.
#
# If you don't have XMMS installed at all, you should comment out
# AM_PATH_XMMS in configure.in.
#
# FLAC uses iconv but not gettext.  iconv requires config.rpath which
# is supplied by gettext, which is copied in by gettextize.  But we
# can't run gettextize since we do not fulfill all it's requirements
# (because we don't use it).  So you may have to:
#
#   cp /usr/share/gettext/config.rpath .
#
# before running autogen.sh
#
# If you are running on OS X and get errors related to the AM_ICONV
# and/or AM_LANGINFO_CODESET macros, replace those 2 lines in
# configure.in with
#
#   AC_DEFINE([HAVE_ICONV], [], [Whether we have libiconv available]) LIBICONV="-liconv"
#   AC_SUBST(LIBICONV)
#
# See also http://lists.xiph.org/pipermail/flac-dev/2007-September/002384.html
#
# Also watchout, if you replace ltmain.sh, there is a bug in some
# versions of libtool (or maybe autoconf) on some platforms where the
# configure-generated libtool does not have $SED defined.  See also:
#
#   http://lists.gnu.org/archive/html/libtool/2003-11/msg00131.html

package="flac"

olddir=`pwd`
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

ACLOCAL_FLAGS="-I m4 $ACLOCAL_FLAGS"

echo "checking for autoconf... "
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

VERSIONGREP="sed -e s/.*[^0-9\.]\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/"
VERSIONMKMAJ="sed -e s/\([0-9][0-9]*\)[^0-9].*/\\1/"
VERSIONMKMIN="sed -e s/.*[0-9][0-9]*\.//"

# do we need automake?
if test -r Makefile.am; then
  AM_OPTIONS=`fgrep AUTOMAKE_OPTIONS Makefile.am`
  AM_NEEDED=`echo $AM_OPTIONS | $VERSIONGREP`
  if test x"$AM_NEEDED" = "x$AM_OPTIONS"; then
    AM_NEEDED=""
  fi
  if test -z $AM_NEEDED; then
    echo -n "checking for automake... "
    AUTOMAKE=automake
    ACLOCAL=aclocal
    if ($AUTOMAKE --version < /dev/null > /dev/null 2>&1); then
      echo "yes"
    else
      echo "no"
      AUTOMAKE=
    fi
  else
    echo -n "checking for automake $AM_NEEDED or later... "
    majneeded=`echo $AM_NEEDED | $VERSIONMKMAJ`
    minneeded=`echo $AM_NEEDED | $VERSIONMKMIN`
    for am in automake-$AM_NEEDED automake$AM_NEEDED \
	automake automake-1.7 automake-1.8 automake-1.9 automake-1.10; do
      ($am --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$am --version < /dev/null | head -n 1 | $VERSIONGREP`
      maj=`echo $ver | $VERSIONMKMAJ`
      min=`echo $ver | $VERSIONMKMIN`
      if test $maj -eq $majneeded -a $min -ge $minneeded; then
        AUTOMAKE=$am
        echo $AUTOMAKE
        break
      fi
    done
    test -z $AUTOMAKE &&  echo "no"
    echo -n "checking for aclocal $AM_NEEDED or later... "
    for ac in aclocal-$AM_NEEDED aclocal$AM_NEEDED \
	aclocal aclocal-1.7 aclocal-1.8 aclocal-1.9 aclocal-1.10; do
      ($ac --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$ac --version < /dev/null | head -n 1 | $VERSIONGREP`
      maj=`echo $ver | $VERSIONMKMAJ`
      min=`echo $ver | $VERSIONMKMIN`
      if test $maj -eq $majneeded -a $min -ge $minneeded; then
        ACLOCAL=$ac
        echo $ACLOCAL
        break
      fi
    done
    test -z $ACLOCAL && echo "no"
  fi
  test -z $AUTOMAKE || test -z $ACLOCAL && {
        echo
        echo "You must have automake installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        exit 1
  }
fi

echo -n "checking for libtool... "
for LIBTOOLIZE in libtoolize glibtoolize nope; do
  ($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 && break
done
if test x$LIBTOOLIZE = xnope; then
  echo "nope."
  LIBTOOLIZE=libtoolize
else
  echo $LIBTOOLIZE
fi
($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

if test -z "$*"; then
        echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

echo "Generating configuration files for $package, please wait...."

echo "  $ACLOCAL $ACLOCAL_FLAGS"
$ACLOCAL $ACLOCAL_FLAGS || exit 1
echo "  $LIBTOOLIZE --automake"
$LIBTOOLIZE --automake || exit 1
echo "  autoheader"
autoheader || exit 1
echo "  $AUTOMAKE --add-missing $AUTOMAKE_FLAGS"
$AUTOMAKE --add-missing $AUTOMAKE_FLAGS || exit 1
echo "  autoconf"
autoconf || exit 1

cd $olddir
$srcdir/configure --enable-maintainer-mode "$@" && echo
