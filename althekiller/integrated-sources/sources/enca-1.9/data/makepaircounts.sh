#! /bin/bash
textdir=/none/tmp/texts
maps=maps
letters=letters
subdirs="belarussian bulgarian russian ukrainian"

dir="$1"
test -n "$dir" || dir="$subdirs"

for d in $dir; do
  c=`ls $d/rawcounts.* | head -n1 | cut -d. -f2`
  echo '[ '$d / $c' ]'
  cat $textdir/$d/* | ./countpair $letters/$c.letters \
    >$d/paircounts.$c
  if test "$d" = "belarussian"; then
    c2=ibm866
    echo '[ '$d / $c2' ]'
    cat $textdir/$d/* | ./xlt $maps/$c.map $maps/$c2-bad.map \
      | ./countpair $letters/$c2-bad.letters \
      >$d/paircounts.$c2
  fi
done


