#!/bin/sh
#$Id: check_opts.sh,v 1.10 2007/12/28 02:11:01 rocky Exp $
# Check cd-info options
if test -z "$srcdir" ; then
  srcdir=`pwd`
fi

if test "X$top_builddir" = "X" ; then
  top_builddir=`pwd`/..
fi

. ${top_builddir}/test/check_common_fn

if test ! -x ../src/cd-info ; then
  exit 77
fi

BASE=`basename $0 .sh`

fname=isofs-m1
i=0
for opt in '-T' '--no-tracks' '-A' '--no-analyze' '-I' '--no-ioctl' \
      '-q' '--quiet' ; do 
  testname=${BASE}$i
  opts="--no-device-info --cue-file ${srcdir}/${fname}.cue $opt --quiet"
  test_cdinfo  "$opts" ${testname}.dump ${srcdir}/${testname}.right
  RC=$?
  check_result $RC "cd-info option test $opt" "../src/cd-info $opts"
  i=`expr $i + 1`
done

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
