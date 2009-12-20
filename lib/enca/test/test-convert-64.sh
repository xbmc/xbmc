#! /bin/sh
# @(#) $Id: test-convert-64.sh,v 1.6 2005/11/24 11:42:47 yeti Exp $
# Purpose: test whether built-in converter works.  We need files larger than
# 64kb, because enca used to have problems with them (the generated big samples
# are between 100k and 600k, depending on original small sample size).
. $srcdir/setup.sh
for l in $TEST_LANGUAGES; do
  # Create big files form the fragments.
  lname=TEST_PAIR_$l
  # Is this POSIX?  But even ash supports it.
  eval lname=$`echo $lname`
  if test "x$lname" != x; then
    src=
    for c in $lname; do
      cp -f $srcdir/$l-s.$c $l-big.$c
      for i in 1 2 3 4 5 6; do
        cat $l-big.$c $l-big.$c $l-big.$c $l-big.$c >test.tmp
        mv -f test.tmp $l-big.$c
      done
      if test -z "$src"; then
        src=$c
      else
        tgt=$c
      fi
    done
    # The test itself.
    DIE_THIS=0
    $ENCA -L $l -x $tgt $l-big.$src || DIE_THIS=1
    diff $l-big.$src $l-big.$tgt >/dev/null || DIE_THIS=1
    if test "$DIE_THIS" = "1"; then
      echo "Conversion $l: $src -> $tgt failed."
      DIE=1
    else
      rm -f $l-big.$src $l-big.$tgt
    fi
  fi
done
. $srcdir/finish.sh

