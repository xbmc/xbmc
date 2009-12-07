#!/bin/sh

#
# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root" 1>&2
	exit 1
fi

set -e

THISDIR=$(pwd)
WORKDIR=workarea

if [ -z "$LH_HOMEDIR" ]; then
	LH_HOMEDIR=$THISDIR/Tools/live-helper

	export LH_BASE="${LH_HOMEDIR}"
	export PATH="${LH_BASE}/helpers:${PATH}"

	export http_proxy="http://127.0.0.1:3142"
	export ftp_proxy="http://127.0.0.1:3142"

	UBUNTUMIRROR_BASEURL="http://mirror.bytemark.co.uk/ubuntu/"
fi

if ! lh -v > /dev/null ; then
	git clone git://live.debian.net/git/live-helper.git
fi

#
# Build needed packages
#
cd buildDEBs
build.sh
cd $THISDIR

#
# Build restricted drivers
#
cd buildRestricted
build.sh
cd $THISDIR

#
# Copy all needed files in place for the real build
#

#TODO

#
# Perform XBMCLive inage build
#
cd buildLive
build.sh
