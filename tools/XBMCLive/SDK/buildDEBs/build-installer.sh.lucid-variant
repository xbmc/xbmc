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


if ! ls xbmclive-installhelpers_*.udeb > /dev/null 2>&1 ; then
	echo "Making xbmclive-installhelpers..."
	cd $THISDIR/xbmclive-installhelpers
	dpkg-buildpackage -rfakeroot -b -uc -us 
	cd $THISDIR
fi

# Rationale: live-base and live-installer have not been synced properly on Lucid (live-base lagging behind), so we need to have a workaround
# Retrieve live_installer sources from Ubuntu's repositories
if ! ls live-installer*.udeb > /dev/null 2>&1 ; then
	echo "Retrieving live_installer sources..."

	repoURL="http://archive.ubuntu.com/ubuntu/pool/main/l/live-installer/"

	latestPackage=$(curl -x "" -s -f $repoURL | grep -o 'live[^"]*.tar.gz' | sort -r -k2 -t_ -n | head -n 1)
	echo " -  Latest package: $latestPackage"
	wget -q "$repoURL$latestPackage"
	if [ "$?" -ne "0" ] || [ ! -f $latestPackage ] ; then
		echo "Needed package (1) not found, exiting..."
		exit 1
	fi
	tar xf $latestPackage

	# Patch to remove unhandled waypoint
	sed -i -e '/final_apt_preferences/s/^waypoint/#waypoint/' live-installer*/debian/live-installer.postinst

	cd live-installer*
	dpkg-buildpackage -rfakeroot -b -uc -us 
	if [ "$?" -ne "0" ]; then
		exit 1
	fi
	cd ..
fi
