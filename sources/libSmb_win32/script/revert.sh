#!/bin/sh
BINDIR=$1
shift

for p in $*; do
 p2=`basename $p`
 if [ -f $BINDIR/$p2.old ]; then
   echo Restoring $BINDIR/$p2.old
   mv $BINDIR/$p2 $BINDIR/$p2.new
   mv $BINDIR/$p2.old $BINDIR/$p2
   rm -f $BINDIR/$p2.new
 else
   echo Not restoring $p
 fi
done

exit 0

