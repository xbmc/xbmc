#!/bin/bash

#      Copyright (C) 2005-2008 Team XBMC
#      http://www.xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html

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
	echo "A required package (live-build) is not available, exiting..."
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
mv $WORKDIR/binary.* .
