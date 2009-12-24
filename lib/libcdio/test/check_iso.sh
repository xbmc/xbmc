#!/bin/sh
#$Id: check_iso.sh.in,v 1.5 2005/01/29 14:50:34 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

if test ! -x ../src/iso-info ; then
  exit 77
fi

BASE=`basename $0 .sh`
fname=copying

if test -n "" ; then
  opts="--quiet ${srcdir}/${fname}.iso --iso9660 "
  test_iso_info  "$opts" ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC 'iso-info basic test' "$ISO_INFO $opts"

  BASE=`basename $0 .sh`
  fname=joliet
  opts="--quiet ${srcdir}/${fname}.iso --iso9660 "
  test_iso_info  "$opts" ${fname}-nojoliet.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC 'iso-info Joliet test' "$cmdline"
  opts="--quiet ${srcdir}/${fname}.iso --iso9660 --no-joliet "
  test_iso_info  "$opts" ${fname}-nojoliet.dump \
                 ${srcdir}/${fname}-nojoliet.right
  RC=$?
  check_result $RC 'iso-info --no-joliet test' "$cmdline"
fi

opts="--image ${srcdir}/${fname}.iso --extract $fname "
test_iso_read  "$opts" ${fname} ${srcdir}/../COPYING
RC=$?
check_result $RC 'iso-read test 1' "$ISO_READ $opts"

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
