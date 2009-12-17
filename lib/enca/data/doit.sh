#! /bin/bash
charsets="$@"
base=$1
language=$(basename $(pwd))
target=$language.h

xlate () {
  ../xlt ../maps/$1.map ../maps/$2.map
}

# This is a recursive call.
if test -z "$base"; then
  for d in `ls */doit.sh | cut -d/ -f1`; do
    echo '[ '$d' ]'
    cd ./$d >/dev/null
    ./doit.sh
    cd .. >/dev/null
  done
  exit 0
fi

# Counts
for cs in $charsets; do
  echo '+'$cs...
  if test "$cs" != "$base"; then
    if test -s $cs.xbase; then
      mv -f $cs.xbase $cs.base
    else
      rm -f $cs.xbase 2>/dev/null
      xlate $base $cs <$base.base >$cs.base
      if test ! -s $cs.base; then
        echo "Cannot create $cs.base" 2>&1
        exit 1
      fi
    fi
  fi
  ../basetoc $cs <$cs.base >$cs.c
done

# Pairs
if test -f paircounts.$base; then
  if test `echo $charsets | wc -w` -gt 8; then
    echo '*** Warning: more than 8 charsets.  Expect coredumps... ***' 1>&2
  fi
  for cs in $charsets; do
    echo '++'$cs...
    if test -f paircounts.$cs; then
      cp paircounts.$cs $cs.pair
    else
      xlate $base $cs <$base.pair >$cs.pair
    fi
    if test ! -s $cs.pair; then
      echo "Cannot create $cs.pair" 2>&1
      exit 1
    fi
    ../pairtoc $cs ../letters/$cs.letters <$cs.pair >$cs.p.c
  done
fi

# Totals
echo =totals...
../totals.pl $charsets
if test ! -s totals.c; then
  echo "Cannot create totals.c" 2>&1
  exit 1
fi

echo '>'$target...
echo "/*****  THIS IS A GENERATED FILE.  DO NOT TOUCH!  *****/" >$target
for cs in $charsets; do
  cat $cs.c >>$target
  if test -s $cs.p.c; then
    cat $cs.p.c >>$target
  fi
  echo >>$target
done
cat totals.c >>$target

echo done.
