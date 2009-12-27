#! /bin/sh
# @(#) $Id: test-guess-short.sh,v 1.6 2003/11/17 12:27:40 yeti Exp $
# Purpose: test charset recognition on tiny text fragments.
. $srcdir/setup.sh
for l in $ALL_TEST_LANGUAGES; do
  $ENCA -p -e -L $l $srcdir/$l-s.* | sed -e "s#^$srcdir/##" >>$TESTNAME.actual || DIE=1
done
. $srcdir/finish.sh
