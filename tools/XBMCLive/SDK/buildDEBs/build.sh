#!/bin/sh

#
# rationale for squashfs-udeb: 
# karmic d-i does not include squashfs module, we need one!
# Lucid has been fixed already:
# https://bugs.launchpad.net/ubuntu/+source/grub-installer/+bug/484832
#
# This is a quick hack to get the installer work on karmic right now, 
# hopefully the fix will be backported to karmic as well
#

THISDIR=$(pwd)
WORKDIR="workarea"

getPackage()
{
	udebListURL="${UBUNTUMIRROR_BASEURL}dists/karmic/main/installer-i386/current/images/udeb.list"

	tmpFile=$(mktemp -q)
	curl -x "" -s -f -o $tmpFile $udebListURL
	if [ "$?" -ne "0" ]; then
		echo "Installer udeb list not found, exiting..."
		exit 1
	fi

	# fs-core-modules-2.6.31-14-generic-di 2.6.31-14.48 i386
	kernelVersion="$(cat $tmpFile | grep fs-core | awk '{ print $2 }')"
	if [ -z "$kernelVersion" ]; then
		echo "Installer kernel version not found, exiting..."
		exit 1
	fi

	# linux-image-2.6.31-14-generic_2.6.31-14.48_i386.deb
	packageName=linux-image-$(echo $kernelVersion | awk -F'.' '{ print $1"."$2"."$3}')-generic_"$kernelVersion"_i386.deb

	packageURL="${UBUNTUMIRROR_BASEURL}pool/main/l/linux/$packageName"

	wget -q $packageURL
	if [ "$?" -ne "0" ]; then
		echo "Needed kernel not found, exiting..."
		exit 1
	fi
	rm $tmpFile

	echo $packageName
}

extractModule()
{
	if [ -d $WORKDIR ]; then
		rm -rf $WORKDIR 
	fi
	mkdir $WORKDIR

	# Extract package
	dpkg -x $1 $WORKDIR

	# Get file from package
	modulesDir=$(ls $WORKDIR/lib/modules/)
	mkdir -p $modulesDir/kernel/fs
	cp -R $WORKDIR/lib/modules/$modulesDir/kernel/fs/squashfs $modulesDir/kernel/fs

	# Remove any previous modules tree in the destination directory
	rm -rf $THISDIR/squashfs-udeb/modules/*

	# Copy the new tree in place
	mkdir $THISDIR/squashfs-udeb/modules
	mv $modulesDir $THISDIR/squashfs-udeb/modules

	# Cleanup
	rm $1
	rm -rf $WORKDIR 
}

makeDEBs()
{
	cd $THISDIR/squashfs-udeb
	dpkg-buildpackage -rfakeroot -b -uc -us 
	cd $THISDIR

	cd $THISDIR/xbmclive-installhelpers
	dpkg-buildpackage -rfakeroot -b -uc -us 
	cd $THISDIR

	cd $THISDIR/live-initramfs-ubuntu
	dpkg-buildpackage -rfakeroot -b -uc -us 
	cd $THISDIR
}

# Get matching package
echo "Selecting and downloading the kernel package..."
packageName=$(getPackage)
if [ ! -f "$packageName" ]; then
	echo "Error retrieving installer kernel, exiting..."
	exit 1
fi

echo "Extracting files..."
extractModule $packageName

echo "Making debs..."
makeDEBs

# Retrieve live_installer from Debian's repositories
# TODO identify & retrieve the latest!
echo "Retrieving live_installer udebs..."
wget -q "http://ftp.uk.debian.org/debian/pool/main/l/live-installer/live-installer_13_i386.udeb"
if [ "$?" -ne "0" ]; then
	echo "Needed package (1) not found, exiting..."
	exit 1
fi

wget -q "http://ftp.uk.debian.org/debian/pool/main/l/live-installer/live-installer-launcher_13_all.deb"
if [ "$?" -ne "0" ]; then
	echo "Needed package (2) not found, exiting..."
	exit 1
fi

