#!/bin/sh

#
# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

set -e

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

if [ -z "$LH_HOMEDIR" ]; then
	LH_HOMEDIR=$THISDIR/Tools/live-helper

	export LH_BASE="${LH_HOMEDIR}"
	export PATH="${LH_BASE}/helpers:${PATH}"
fi

if ! which lh > /dev/null ; then
	cd $THISDIR/Tools
	git clone git://live.debian.net/git/live-helper.git
	if [ "$?" -ne "0" ]; then
		exit 1
	fi
	cd $THISDIR
fi

#
# Build needed packages
#
cd buildDEBs
./build.sh
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd $THISDIR

#
# Build restricted drivers
#
cd buildRestricted
./build.sh
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd $THISDIR

#
# Copy all needed files in place for the real build
#

#TODO

#
# Perform XBMCLive inage build
#
cd buildLive
./build.sh
