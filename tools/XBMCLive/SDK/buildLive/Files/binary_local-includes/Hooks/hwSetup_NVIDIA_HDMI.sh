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

NvidiaHDMIFirstGen=$(lspci -nn | grep '0403' | grep '10de:0ac0') #MCP79 High Definition Audio 
NvidiaHDMISecondGen=$(lspci -nn | grep '0403' | grep '10de:0be3') #MCP79 High Definition Audio 

if [ ! -n "$NvidiaHDMISecondGen" ] && [ ! -n "$NvidiaHDMIFirstGen" ] ; then
        exit 0
fi

HDMICARD=$(aplay -l | grep 'NVIDIA HDMI' | awk -F: '{ print $1 }' | awk '{ print $2 }')
HDMIDEVICE=$(aplay -l | grep 'NVIDIA HDMI' | awk -F: '{ print $2 }' | awk '{ print $5 }')

if [ -n "$NvidiaHDMIFirstGen" ] ; then
	# "VT1708S Digital"
	# "ALC662 rev1 Digital"
	# "ALC1200 Digital"
	# "ALC662 Digital"
	# "ALC889A Digital"
	DIGITALCONTROL="VT1708S Digital\|ALC662 rev1 Digital\|ALC1200 Digital\|ALC662 Digital\|ALC889A Digital"
fi

if [ -n "$NvidiaHDMISecondGen" ] ; then
	# "ALC888 Digital"
	DIGITALCONTROL="ALC888 Digital"
fi


#
# Setup kernel module parameters
#

if [ -n "$NvidiaHDMISecondGen" ] ; then
	if ! grep -i -q snd-hda-intel /etc/modprobe.d/alsa-base.conf ; then
		if [ $HDMICARD,$HDMIDEVICE == 1,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
		elif [ $HDMICARD,$HDMIDEVICE == 0,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xfff2' >> /etc/modprobe.d/alsa-base.conf
		elif [ $HDMICARD,$HDMIDEVICE == 2,3 ]; then
			echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
	#	else
	#		echo "No matching value detected"
		fi
	fi
fi

#
# Setup .asoundrc
#

xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')

if [ ! -f /home/$xbmcUser/.asoundrc ] ; then
	cat > /home/$xbmcUser/.asoundrc << 'EOF'
pcm.!default {
        type plug
        slave {
                pcm "both"
        }
}
pcm.both {
        type route
        slave {
                pcm multi
                channels 4
        }
        ttable.0.0 1.0
        ttable.1.1 1.0
        ttable.0.2 1.0
        ttable.1.3 1.0
}
pcm.multi {
        type multi
        slaves.a {
                pcm "tv"
                channels 2
        }
        slaves.b {
                pcm "dmixrec"
                channels 2
        }
        bindings.0.slave a
        bindings.0.channel 0
        bindings.1.slave a
        bindings.1.channel 1
        bindings.2.slave b
        bindings.2.channel 0
        bindings.3.slave b
        bindings.3.channel 1
}
pcm.dmixrec {
    type dmix
    ipc_key 1024
    slave {
        pcm "receiver"
        period_time 0
        period_size 1024
        buffer_size 8192
        rate 48000
     }
     bindings {
        0 0
        1 1
     }
}
pcm.tv {
        type hw
        =HDMICARD=
        =HDMIDEVICE=
        channels 2
}
pcm.receiver {
        type hw
        =DIGITALCARD=
        =DIGITALDEVICE=
        channels 2
}
EOF

	DIGITALCARD=$(aplay -l | grep $DIGITALCONTROL | awk -F: '{ print $1 }' | awk '{ print $2 }')
	DIGITALDEVICE=$(aplay -l | grep $DIGITALCONTROL | awk -F: '{ print $2 }' | awk '{ print $5 }')

	sed -i "s/=HDMICARD=/card $HDMICARD/g" /home/$xbmcUser/.asoundrc
	sed -i "s/=HDMIDEVICE=/device $HDMIDEVICE/g" /home/$xbmcUser/.asoundrc

	sed -i "s/=DIGITALCARD=/card $DIGITALCARD/g" /home/$xbmcUser/.asoundrc
	sed -i "s/=DIGITALDEVICE=/device $DIGITALDEVICE/g" /home/$xbmcUser/.asoundrc

	chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.asoundrc
fi

#
# Unmute digital output
#

/usr/bin/amixer -q -c 0 sset 'IEC958 Default PCM',0 unmute > /dev/null
/usr/bin/amixer -q -c 0 sset 'IEC958',0 unmute > /dev/null
/usr/bin/amixer -q -c 0 sset 'IEC958',1 unmute > /dev/null
