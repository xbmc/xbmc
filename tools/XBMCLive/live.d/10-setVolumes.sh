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

kernelParams=$(cat /proc/cmdline)

subString=${kernelParams##*xbmc=}
xbmcParams=${subString%% *}

activationToken="setvolume"

# if strings are the same the token is NOT part of the parameters list
# here we want to stop script if the token is NOT there
if [ "$xbmcParams" = "${xbmcParams%$activationToken*}" ] ; then
	exit 0
fi

/usr/bin/setAlsaVolumes 90

exit 0
