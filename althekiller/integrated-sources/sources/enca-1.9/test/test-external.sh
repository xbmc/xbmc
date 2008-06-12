#! /bin/sh
# @(#) $Id: test-external.sh,v 1.4 2003/11/17 12:27:39 yeti Exp $
# Purpose: test whether external convertor is executed with correct arguments,
# whether enca fails when it cannot find it iff there's something to convert,
# whether convertor errors are correctly propagated
. $srcdir/setup.sh
if $ENCA --list convertors | grep '^extern$' >/dev/null; then
  TEST_TEXT=$srcdir/cs-s.iso88592
  cat $TEST_TEXT >$TESTNAME.tmp
  OPTS="-L cs -x 1250"
  $ENCA -E $srcdir/dummy-convertor.sh $OPTS -C extern $TESTNAME.tmp >>$TESTNAME.actual || DIE=1
  $ENCA -E tolstoj-mumble-mumble $OPTS -C built-in <$TEST_TEXT >/dev/null || DIE=1
  $ENCA -E tolstoj-mumble-mumble $OPTS -C extern <$TEST_TEXT >/dev/null 2>/dev/null && DIE=1
  $ENCA -E '' $OPTS -C extern <$TEST_TEXT >/dev/null 2>/dev/null && DIE=1
  $ENCA -E $srcdir/failing-convertor.sh $OPTS -C extern $TESTNAME.tmp 2>/dev/null && DIE=1
  $ENCA -E $srcdir/failing-convertor2.sh $OPTS -C extern $TESTNAME.tmp 2>/dev/null && DIE=1
else
  E77=1
fi
. $srcdir/finish.sh
