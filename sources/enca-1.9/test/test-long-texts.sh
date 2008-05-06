#! /bin/sh
# @(#) $Id: test-long-texts.sh,v 1.5 2003/11/17 12:27:40 yeti Exp $
# Purpose: check whether enca correctly prints license, help, version.
. $srcdir/setup.sh
$ENCA --version >$TESTNAME.actual || DIE=1
version=`grep '^AC_INIT' $top_srcdir/configure.ac | sed -e 's/AC_INIT(Enca, \([0-9.]*\).*/\1/' -e 's/\./\\./'`
grep '^enca '$version $TESTNAME.actual >/dev/null || DIE=1
$ENCA --license >$TESTNAME.actual || DIE=1
diff $top_srcdir/COPYING $TESTNAME.actual || DIE=1
$ENCA --help >$TESTNAME.actual || DIE=1
diff $top_builddir/src/HELP $TESTNAME.actual || DIE=1
grep '^Report bugs to .*<.*@.*>' $TESTNAME.actual >/dev/null || DIE=1
. $srcdir/finish.sh
