#! /bin/sh
# @(#) $Id: test-default-cs.sh,v 1.8 2005/11/24 11:42:47 yeti Exp $
# Purpose: test whether DEFAULT_CHARSET works.
. $srcdir/setup.sh
for l in $TEST_LANGUAGES; do
  lname=TEST_PAIR_$l
  # Is this POSIX?  But even ash supports it.
  eval lname=$`echo $lname`
  if test "x$lname" != x; then
    src=
    for c in $lname; do
      if test -z "$src"; then
        src=$c
      else
        tgt=$c
      fi
    done
    cat $srcdir/$l-s.$src >$TESTNAME-$l.tmp
    DEFAULT_CHARSET=$tgt
    # This is necessary when $ENCA is in fact a libtool script
    export DEFAULT_CHARSET
    DIE_THIS=0
    $ENCA -c -L $l $TESTNAME-$l.tmp || DIE=1
    diff $TESTNAME-$l.tmp $srcdir/$l-s.$tgt >/dev/null || DIE_THIS=1
    if test "$DIE_THIS" = "1"; then
      echo "Conversion $l: $src -> $tgt failed."
      DIE=1
    else
      rm -f $TESTNAME-$l.tmp
    fi
  fi
done
. $srcdir/finish.sh
