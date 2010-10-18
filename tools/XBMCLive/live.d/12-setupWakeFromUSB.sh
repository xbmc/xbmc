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

findUSBPort() {
	local FD=7
	local tmpFile=$(mktemp)
	local eof=0
	local line
	local usbPort=""
	local pciDevice=""
	local inBlock="0"
	local token=$1

	lshal > $tmpFile

	# Open files.
	eval exec "$FD<$tmpFile"

	while [ $eof -eq 0 ]
	do
		if read line <&$FD; then
			if [ -n "$(echo $line | grep -i $token)" ]; then
				if [ "$inBlock" = "0" ]; then
					inBlock="1"
					token="linux.sysfs_path"
				else
					pciDevice=$(echo $line | awk -F/ '{ print $5}')
					eof=1
				fi
			fi
		else
			eof=1
		fi
	done

	if [ -n "$pciDevice" ] ; then
		usbPort=$(cat /proc/acpi/wakeup | grep $pciDevice | awk '{ print $1}')
	fi

	echo $usbPort
}


kernelParams=$(cat /proc/cmdline)

subString=${kernelParams##*xbmc=}
xbmcParams=${subString%% *}

activationToken="wakeOnUSBRemote"

# if strings are the same the token is NOT part of the parameters list
# here we want to stop script if the token is NOT there
if [ "$xbmcParams" = "${xbmcParams%$activationToken*}" ] ; then
	exit
fi

lircDriver=$(dmesg | grep usbcore | grep -i 'lirc' | sed -e "s/.* \(lirc*\)/\1/" | head -n 1)
if [ ! -n "$lircDriver" ] ; then
	# No lirc driver loaded
	exit 0
fi

# Wait for udevtrigger to settle down (if any)
udevTriggerPID=$(ps x | grep udevtrigger | grep -v grep | cut -f2 -d ' ')
if [ -n "udevTriggerPID" }; then
	while test -d /proc/$udevTriggerPID; do sleep 1; done;
fi

usbPort=$(findUSBPort "${lircDriver}")

if [ -z "$usbPort" ]; then
	# No USB Remote found
	exit 0
fi

usbStatus=`cat /proc/acpi/wakeup | grep $usbPort | awk {'print $3}'`
if [ "$usbStatus" = "disabled" ]; then
	echo $usbPort > /proc/acpi/wakeup
	echo -1 >/sys/module/usbcore/parameters/autosuspend
fi

exit 0
