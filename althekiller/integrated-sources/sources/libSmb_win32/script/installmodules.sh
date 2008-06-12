#!/bin/sh

INSTALLPERMS=$1
DESTDIR=$2
prefix=`echo $3 | sed 's/\/\//\//g'`
LIBDIR=`echo $4 | sed 's/\/\//\//g'`
shift
shift
shift
shift

for d in $prefix $LIBDIR; do
if [ ! -d $DESTDIR/$d ]; then
mkdir $DESTDIR/$d
if [ ! -d $DESTDIR/$d ]; then
  echo Failed to make directory $DESTDIR/$d
  exit 1
fi
fi
done

for p in $*; do
 p2=`basename $p`
 echo Installing $p as $DESTDIR/$LIBDIR/$p2
 cp -f $p $DESTDIR/$LIBDIR/
 chmod $INSTALLPERMS $DESTDIR/$LIBDIR/$p2
done

exit 0
