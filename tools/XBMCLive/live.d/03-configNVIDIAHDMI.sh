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
# Nvidia ION detection
#

NvidiaHDMIFirstGen=$(lspci -nn | grep '0403' | grep '10de:0ac0') #MCP79 High Definition Audio
NvidiaHDMISecondGen=$(lspci -nn | grep '0403' | grep '10de:0be3') #MCP79 High Definition Audio

if [ ! -n "$NvidiaHDMISecondGen" ] && [ ! -n "$NvidiaHDMIFirstGen" ] ; then
	exit 0
fi

#
# Set Nvidia HDMI variables
#

HDMICARD=$(aplay -l | grep 'NVIDIA HDMI' -m1 | awk -F: '{ print $1 }' | awk '{ print $2 }')
HDMIDEVICE=$(aplay -l | grep 'NVIDIA HDMI' -m1 | awk -F: '{ print $2 }' | awk '{ print $5 }')

#
# Bails out if we don't have digital outputs
#
if [ -z $HDMICARD ] || [ -z $HDMIDEVICE ]; then
	exit 0
fi

#
# Restart only if needed
#
restartALSA=""

#
# Setup kernel module parameters
#

if [ -n "$NvidiaHDMISecondGen" ] ; then
	if ! grep -i -q snd-hda-intel /etc/modprobe.d/alsa-base.conf ; then
		if [ $HDMICARD,$HDMIDEVICE == 1,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
			restartALSA="1"
                elif [ $HDMICARD,$HDMIDEVICE == 0,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xfff2' >> /etc/modprobe.d/alsa-base.conf
			restartALSA="1"
                elif [ $HDMICARD,$HDMIDEVICE == 2,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
			restartALSA="1"
		fi
	fi
fi

if [ -n "$restartALSA" ] ; then
	#
	# Restart Alsa
	#

	alsa-utils stop &> /dev/null
	alsa force-reload &> /dev/null
	alsa-utils start &> /dev/null

	#
	# Unmute digital output
	#

	/usr/bin/amixer -q -c 0 sset 'IEC958 Default PCM',0 unmute &> /dev/null
	/usr/bin/amixer -q -c 0 sset 'IEC958',0 unmute &> /dev/null
	/usr/bin/amixer -q -c 0 sset 'IEC958',1 unmute &> /dev/null
	/usr/bin/amixer -c 1 sset 'IEC958' unmute &> /dev/null

	#
	# Store alsa settings
	#

	alsactl store &> /dev/null
fi

exit 0

