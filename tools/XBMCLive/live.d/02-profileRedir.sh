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

#
# Exiting if run on a live system only
#
if ! grep "boot=live" /proc/cmdline ; then
	exit 0
fi

#
# Exiting if booting from Live CD
#
if [ "$(mount | grep iso9660)" ]; then
	exit 0
fi

kernelParams=$(cat /proc/cmdline)

subString=${kernelParams##*xbmc=}
xbmcParams=${subString%% *}

activationToken="noredir"

# if strings are NOT the same the token is part of the parameters list
# here we want to stop script if the token is there
if [ "$xbmcParams" != "${xbmcParams%$activationToken*}" ] ; then
	exit 0
fi

BOOTMEDIAMOUNTPOINT=$1

if [ -n "$BOOTMEDIAMOUNTPOINT" ]; then
	if [ ! -d $BOOTMEDIAMOUNTPOINT/dotXBMC ]; then
		mkdir $BOOTMEDIAMOUNTPOINT/dotXBMC
	fi
	if [ -d /home/xbmc/.xbmc ]; then
		if [ -L /home/xbmc/.xbmc ]; then
			rm /home/xbmc/.xbmc
		else
			mv /home/xbmc/.xbmc /home/xbmc/.xbmc.previous
		fi
	fi
	ln -s $BOOTMEDIAMOUNTPOINT/dotXBMC /home/xbmc/.xbmc
fi

exit 0
