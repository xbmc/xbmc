#!/bin/sh

INSTALLPERMS=$1
DESTDIR=`echo $2 | sed 's/\/\//\//g'`
shift
shift

for dir in $@; do
	DIRNAME=`echo $dir | sed 's/\/\//\//g'`
	if [ ! -d $DESTDIR/$DIRNAME ]; then
		mkdir -m $INSTALLPERMS -p $DESTDIR/$DIRNAME
	fi

	if [ ! -d $DESTDIR/$DIRNAME ]; then
		echo "Failed to make directory $DESTDIR/$DIRNAME "
		exit 1
	fi
done
