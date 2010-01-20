#! /bin/bash
test -d letters || mkdir letters
for f in maps/*.map; do
  cs=`basename $f .map`
  echo $cs
   ./findletters $f <Letters >letters/$cs.letters
done
