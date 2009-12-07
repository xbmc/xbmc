#!/bin/bash
#
# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

THISDIR=$(pwd)
WORKDIR=workarea

. $THISDIR/getInstallers.sh
. $THISDIR/mkConfig.sh


build()
{
	cd $THISDIR/$WORKDIR

	lh bootstrap
	lh chroot

	# safeguard against crashes
	lh chroot_devpts remove
	lh chroot_proc remove
	lh chroot_sysfs remove
	
	for modulesdir in chroot/lib/modules/*
	do
		umount $modulesdir/volatile &> /dev/null
	done

	cd $THISDIR
}

if ! lh -v > /dev/null ; then
	echo "A required package (live-helper) is not available, exiting..."
	exit
fi

#
#
#

# Clean any previous run
rm -rf $WORKDIR

# Get latest installers if instructed
getInstallers

# Create config tree
makeConfig

# Create chroot and build drivers
build

# Get files from chroot
cp $WORKDIR/chroot/tmp/*.ext3 .
