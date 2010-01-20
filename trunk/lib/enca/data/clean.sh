#! /bin/bash
for d in `ls */doit.sh | cut -d/ -f1`; do
  echo '[ '$d' ]'
  cd ./$d >/dev/null
  test -f "xlt.c" && exit 1
  base=`ls rawcounts.* | head -n1 | cut -d. -f2`
  rm -vf *.c *.pair
  ls *.base *.xbase | grep -v '^'$base'\.' | xargs rm -vf
  cd .. >/dev/null
done
rm *~ 2>/dev/null
