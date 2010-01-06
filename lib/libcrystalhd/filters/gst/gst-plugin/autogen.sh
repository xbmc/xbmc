#!/bin/sh
# you can either set the environment variables AUTOCONF and AUTOMAKE
# to the right versions, or leave them unset and get the RedHat 7.3 defaults

DIE=0
package=gst-plugin
srcfile=src/main.c

# autogen.sh helper functions (copied from GStreamer's common/ CVS module)
if test ! -f ./gst-autogen.sh;
then
  echo There is something wrong with your source tree.
  echo You are either missing ./gst-autogen.sh or not
  echo running autogen.sh from the top-level source
  echo directory.
  exit 1
fi
. ./gst-autogen.sh

CONFIGURE_DEF_OPT='--enable-maintainer-mode --enable-debug'

autogen_options $@

echo -n "+ check for build tools"
if test ! -z "$NOCHECK"; then echo " skipped"; else  echo; fi
version_check "autoconf" "$AUTOCONF autoconf autoconf259 autoconf257 autoconf-2.54 autoconf-2.53 autoconf-2.52" \
              "ftp://ftp.gnu.org/pub/gnu/autoconf/" 2 52 || DIE=1
version_check "automake" "$AUTOMAKE automake automake-1.9 automake19 automake-1.7 automake-1.6 automake-1.5" \
              "ftp://ftp.gnu.org/pub/gnu/automake/" 1 7 || DIE=1
###version_check "autopoint" "autopoint" \
###              "ftp://ftp.gnu.org/pub/gnu/gettext/" 0 11 5 || DIE=1
version_check "libtoolize" "$LIBTOOLIZE libtoolize glibtoolize" \
              "ftp://ftp.gnu.org/pub/gnu/libtool/" 1 5 0 || DIE=1
version_check "pkg-config" "" \
              "http://www.freedesktop.org/software/pkgconfig" 0 8 0 || DIE=1

die_check $DIE

autoconf_2_52d_check || DIE=1
aclocal_check || DIE=1
autoheader_check || DIE=1

die_check $DIE

# if no arguments specified then this will be printed
if test -z "$*"; then
  echo "+ checking for autogen.sh options"
  echo "  This autogen script will automatically run ./configure as:"
  echo "  ./configure $CONFIGURE_DEF_OPT"
  echo "  To pass any additional options, please specify them on the $0"
  echo "  command line."
fi

tool_run "$aclocal" "-I m4/ $ACLOCAL_FLAGS"
tool_run "$libtoolize" "--copy --force"
tool_run "$autoheader"
tool_run "$autoconf"
tool_run "$automake" "-a -c"

# if enable exists, add an -enable option for each of the lines in that file
if test -f enable; then
  for a in `cat enable`; do
    CONFIGURE_FILE_OPT="--enable-$a"
  done
fi

# if disable exists, add an -disable option for each of the lines in that file
if test -f disable; then
  for a in `cat disable`; do
    CONFIGURE_FILE_OPT="$CONFIGURE_FILE_OPT --disable-$a"
  done
fi

test -n "$NOCONFIGURE" && {
  echo "+ skipping configure stage for package $package, as requested."
  echo "+ autogen.sh done."
  exit 0
}

echo "+ running configure ... "
test ! -z "$CONFIGURE_DEF_OPT" && echo "  ./configure default flags: $CONFIGURE_DEF_OPT"
test ! -z "$CONFIGURE_EXT_OPT" && echo "  ./configure external flags: $CONFIGURE_EXT_OPT"
test ! -z "$CONFIGURE_FILE_OPT" && echo "  ./configure enable/disable flags: $CONFIGURE_FILE_OPT"
echo

./configure $CONFIGURE_DEF_OPT $CONFIGURE_EXT_OPT $CONFIGURE_FILE_OPT || {
        echo "  configure failed"
        exit 1
}

echo "Now type 'make' to compile $package."
