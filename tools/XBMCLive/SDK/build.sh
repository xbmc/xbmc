#!/bin/sh

#
# Depencency list: git-core debootstrap asciidoc docbook-xsl curl build-essential
#

#
# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

# Clean our mess on exiting
cleanup()
{
	if [ -n "$WORKPATH" ]; then
		if [ -z "$KEEP_WORKAREA" ]; then
			echo "Cleaning workarea..." 
			rm -rf $WORKPATH
			if [ -f $THISDIR/binary.iso ]; then
				chmod 777 $THISDIR/binary.* 
			fi
			echo "All clean"
		fi
	fi
}
trap 'cleanup' EXIT TERM INT


THISDIR=$(pwd)
WORKDIR=workarea
WORKPATH=$THISDIR/$WORKDIR
export WORKPATH

if [ -d "$WORKPATH" ]; then
	echo "Deleting old workarea..." 
	rm -rf $WORKPATH
fi
mkdir $WORKPATH

if ls $THISDIR/binary.* > /dev/null 2>&1; then
	rm -rf $THISDIR/binary.*
fi

echo "Creating workarea..." 

# cp all (except svn directories) into workarea
rsync -r -l --exclude=.svn --exclude=$WORKDIR . $WORKDIR

if ! which lh > /dev/null ; then
	cd $WORKPATH/Tools
	if [ ! -d live-helper ]; then
		git clone git://live.debian.net/git/live-helper.git
		if [ "$?" -ne "0" ]; then
			exit 1
		fi

		# Fix for missing directory for karmic d-i, to be removed when fixed upstream!
		cd live-helper/data/debian-cd
		ln -s lenny karmic
	fi

	LH_HOMEDIR=$WORKPATH/Tools/live-helper

	export LH_BASE="${LH_HOMEDIR}"
	export PATH="${LH_BASE}/helpers:${PATH}"

	cd $THISDIR
fi

echo "Start building..."

# Execute hooks if env variable is defined
if [ -n "$SDK_BUILDHOOKS" ]; then
	for hook in $SDK_BUILDHOOKS; do
		if [ -x $hook ]; then
			$hook
		fi
	done
fi

#
# Build needed packages
#
cd $WORKPATH/buildDEBs
./build.sh
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd $THISDIR

#
# Build binary drivers
#
cd $WORKPATH/buildBinaryDrivers
./build.sh
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd $THISDIR

#
# Copy all needed files in place for the real build
#
mkdir -p $WORKPATH/buildLive/Files/chroot_local-packages &> /dev/null
mkdir -p $WORKPATH/buildLive/Files/binary_local-udebs &> /dev/null
mkdir -p $WORKPATH/buildLive/Files/chroot_local-includes/root &> /dev/null

if ! ls $WORKPATH/buildDEBs/live-initramfs*.* > /dev/null 2>&1; then
        echo "Files missing (1), exiting..."
        exit 1
fi
cp $WORKPATH/buildDEBs/live-initramfs*.* $WORKPATH/buildLive/Files/chroot_local-packages

if [ -z "$DONOTINCLUDEINSTALLER" ]; then
	if ! ls $WORKPATH/buildDEBs/squashfs-udeb*.* > /dev/null 2>&1; then
		echo "Files missing (2), exiting..."
		exit 1
	fi
	cp $WORKPATH/buildDEBs/squashfs-udeb*.* $WORKPATH/buildLive/Files/binary_local-udebs

	if ! ls $WORKPATH/buildDEBs/live-installer*.* > /dev/null 2>&1; then
		echo "Files missing (3), exiting..."
		exit 1
	fi
	cp $WORKPATH/buildDEBs/live-installer*.* $WORKPATH/buildLive/Files/binary_local-udebs

	if ! ls $WORKPATH/buildDEBs/xbmclive-installhelpers*.* > /dev/null 2>&1; then
		echo "Files missing (4), exiting..."
		exit 1
	fi
	cp $WORKPATH/buildDEBs/xbmclive-installhelpers*.* $WORKPATH/buildLive/Files/binary_local-udebs
fi

if [ -z "$DONOTBUILDRESTRICTEDDRIVERS" ]; then
	mkdir -p $WORKPATH/buildLive/Files/binary_local-includes/live/restrictedDrivers &> /dev/null

	if ! ls $WORKPATH/buildBinaryDrivers/*.ext3 > /dev/null 2>&1; then
		echo "Files missing (5), exiting..."
		exit 1
	fi
	cp $WORKPATH/buildBinaryDrivers/*.ext3 $WORKPATH/buildLive/Files/binary_local-includes/live/restrictedDrivers
fi

if ! ls $WORKPATH/buildBinaryDrivers/crystalhd.tar > /dev/null 2>&1; then
        echo "Files missing (6), exiting..."
        exit 1
fi
cp $WORKPATH/buildBinaryDrivers/crystalhd.tar $WORKPATH/buildLive/Files/chroot_local-includes/root

#
# Perform XBMCLive image build
#
cd $WORKPATH/buildLive
./build.sh
cd $THISDIR

mv $WORKPATH/buildLive/binary.* .
chmod 777 *.iso
