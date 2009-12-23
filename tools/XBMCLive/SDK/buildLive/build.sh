#!/bin/bash

THISDIR=$(pwd)
WORKDIR=workarea

. $THISDIR/mkConfig.sh

build()
{
	lh build

	# safeguard against crashes
	lh chroot_devpts remove
	lh chroot_proc remove
	lh chroot_sysfs remove
	
	for modulesdir in chroot/lib/modules/*
	do
		umount $modulesdir/volatile &> /dev/null
	done
}

if ! which lh > /dev/null ; then
	echo "A required package (live-helper) is not available, exiting..."
	exit 1
fi

#
#
#

# Clean any previous run
rm -rf *.iso &> /dev/null

rm -rf $WORKDIR &> /dev/null
mkdir -p "$THISDIR/$WORKDIR"
cd "$THISDIR/$WORKDIR"

# Create config tree
makeConfig

# Create chroot and build drivers
build

cd $THISDIR

# Get files from chroot
mv $WORKDIR/binary.iso .
