#!/bin/sh

if [ $# = 0 ]; then
	echo "Usage: $0 <directory>"
	exit
fi
if [ ! -d $1 ]; then
	echo "Directory not found, exiting..."
	exit
fi

pushd .
cd $1
dpkg-buildpackage -rfakeroot -b -uc -us
popd
