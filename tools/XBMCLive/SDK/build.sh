#!/bin/sh

#
# Depencency list: git-core apt-cacher-ng debootstrap asciidoc docbook-xsl curl build-essential
#

#
# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

# set -e

# Using apt-cacher(-ng) to speed up apt-get downloads
export APT_HTTP_PROXY="http://127.0.0.1:3142"
export APT_FTP_PROXY="http://127.0.0.1:3142"

# We use apt-cacher when retrieving d-i udebs, too
export http_proxy="http://127.0.0.1:3142"
export ftp_proxy="http://127.0.0.1:3142"

# Closest Ubuntu mirror
export UBUNTUMIRROR_BASEURL="http://mirror.bytemark.co.uk/ubuntu/"

THISDIR=$(pwd)
WORKDIR=workarea
WORKPATH=$THISDIR/$WORKDIR

if [ -d "$WORKPATH" ]; then
	rm -rf $WORKPATH
fi
mkdir $WORKPATH

# cp all (except svn directories) into workarea
rsync -r --exclude=.svn --exclude=$WORKDIR . $WORKDIR

if ! which lh > /dev/null ; then
	cd $WORKPATH/Tools
	git clone git://live.debian.net/git/live-helper.git
	if [ "$?" -ne "0" ]; then
		cd $THISDIR
		rm -rf $WORKPATH
		exit 1
	fi

	# Fix for missing directory for karmic d-i, to be removed when fixed upstream!
	cd live-helper/data/debian-cd
	ln -s lenny karmic

	LH_HOMEDIR=$WORKPATH/Tools/live-helper

	export LH_BASE="${LH_HOMEDIR}"
	export PATH="${LH_BASE}/helpers:${PATH}"

	cd $THISDIR
fi

#
# Build needed packages
#
cd $WORKPATH/buildDEBs
./build.sh
if [ "$?" -ne "0" ]; then
	rm -rf $WORKPATH
	exit 1
fi
cd $THISDIR

#
# Build restricted drivers
#
cd $WORKPATH/buildRestricted
./build.sh
if [ "$?" -ne "0" ]; then
	rm -rf $WORKPATH
	exit 1
fi
cd $THISDIR

#
# Copy all needed files in place for the real build
#
mkdir -p $WORKPATH/buildLive/Files/chroot_local-packages &> /dev/null
mkdir -p $WORKPATH/buildLive/Files/binary_local-udebs &> /dev/null
mkdir -p $WORKPATH/buildLive/Files/binary_local-includes/live/restrictedDrivers &> /dev/null

cp $WORKPATH/buildDEBs/live-initramfs*.* $WORKPATH/buildLive/Files/chroot_local-packages
cp $WORKPATH/buildDEBs/squashfs-udeb*.* $WORKPATH/buildLive/Files/binary_local-udebs
cp $WORKPATH/buildDEBs/live-installer*.* $WORKPATH/buildLive/Files/binary_local-udebs
cp $WORKPATH/buildDEBs/xbmclive-installhelpers*.* $WORKPATH/buildLive/Files/binary_local-udebs

cp $WORKPATH/buildRestricted/*.ext3 $WORKPATH/buildLive/Files/binary_local-includes/live/restrictedDrivers

#
# Perform XBMCLive image build
#
cd $WORKPATH/buildLive
./build.sh
cd $THISDIR

mv $WORKPATH/buildLive/binary.iso .
chmod 777 *.iso

rm -rf $WORKPATH
