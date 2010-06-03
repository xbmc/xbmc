#/bin/bash

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

restoreAutomount=0
CHECKGCONFDIR="$(gconftool-2 --dir-exists=/apps/nautilus/preferences)"
if [ "${CHECKGCONFDIR}" = "0" ]; then
	CHECKAUTOMOUNT="$(gconftool-2 --get /apps/nautilus/preferences/media_automount)"
	if [ "${CHECKAUTOMOUNT}" = "true" ]; then
		restoreAutomount=1
		echo "Please disconnect any USB disks and press a key when done..."
		read choice
		echo Disabling USB automount...
		gconftool-2 --set /apps/nautilus/preferences/media_automount --type=bool false
		echo "Please connect the USB disk and press a key when done..."
		read choice
	fi
fi

sudo python ./installXBMC -l ./XBMCLive.log -i ./XBMCLive.iso

if [ "$restoreAutomount" = "1" ]; then
	echo Restoring USB automount...
	gconftool-2 --set /apps/nautilus/preferences/media_automount --type=bool true
fi
