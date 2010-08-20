#!/bin/bash

#      Copyright (C) 2005-2010 Team XBMC
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

# Temporary fix until package in PPA is created

mkdir -p $WORKPATH/buildLive/Files/chroot_local-packages &> /dev/null

if ! ls $WORKPATH/plymouth-theme-xbmc-logo*.deb > /dev/null 2>&1; then
	cd $WORKPATH
	svn checkout https://xbmc.svn.sourceforge.net/svnroot/xbmc/trunk/tools/XBMCLive/PlymouthThemes/plymouth-theme-xbmc-logo
	if [ "$?" -ne "0" ]; then
		echo "Error retrieving Plymouth theme files, exiting..."
		exit 1
	fi

	cd plymouth-theme-xbmc-logo
	dpkg-buildpackage -rfakeroot -b -uc -us 
	if [ "$?" -ne "0" ]; then
		exit 1
	fi
	cd ..
fi

cp $WORKPATH/plymouth-theme-xbmc-logo*.deb $WORKPATH/buildLive/Files/chroot_local-packages
