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

cat > /usr/bin/hwSetupNvidiaHDMI.sh << 'BLOB'
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

if [ -n "$NvidiaHDMIFirstGen" ] ; then
	# "VT1708S Digital"
	# "ALC662 rev1 Digital"
	# "ALC1200 Digital"
	# "ALC662 Digital"
	# "ALC889A Digital"
	# "ALC888 Digital"
	DIGITALCONTROL="VT1708S Digital\|ALC662 rev1 Digital\|ALC1200 Digital\|ALC662 Digital\|ALC889A Digital\|ALC888 Digital"
fi

if [ -n "$NvidiaHDMISecondGen" ] ; then
	# "ALC887 Digital"
	# "ALC888 Digital"
	DIGITALCONTROL="ALC888 Digital\|ALC887 Digital"
fi

#
# Retrieve Digital Settings before .asoundrc creation
#

DIGITALCARD=$(aplay -l | grep "$DIGITALCONTROL" | awk -F: '{ print $1 }' | awk '{ print $2 }')
DIGITALDEVICE=$(aplay -l | grep "$DIGITALCONTROL" | awk -F: '{ print $2 }' | awk '{ print $5 }')

ANALOGCARD=$(aplay -l | grep 'Analog' -m1 | awk -F: '{ print $1 }' | awk '{ print $2 }')
ANALOGDEVICE=$(aplay -l | grep 'Analog' -m1 | awk -F: '{ print $2 }' | awk '{ print $5 }')

#
# Bails out if we don't have digital outputs
#
if [ -z $HDMICARD ] || [ -z $HDMIDEVICE ] || [ -z $DIGITALCARD ] || [ -z $DIGITALDEVICE ]; then
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

#
# Setup .asoundrc
#
xbmcUser=xbmc
# Read configuration variable file if it is present
[ -r /etc/default/xbmc-live ] && . /etc/default/xbmc-live
if ! getent passwd $xbmcUser >/dev/null; then
	xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')
fi

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
                channels 6
        }
        ttable.0.0 1.0
        ttable.1.1 1.0
        ttable.0.2 1.0
        ttable.1.3 1.0
        ttable.0.4 1.0
        ttable.1.5 1.0
}

pcm.multi {
        type multi
        slaves.a {
                pcm "hdmi_hw"
                channels 2
        }
        slaves.b {
                pcm "digital_hw"
                channels 2
        }
        slaves.c {
                pcm "analog_hw"
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
        bindings.4.slave c
        bindings.4.channel 0
        bindings.5.slave c
        bindings.5.channel 1
}

pcm.hdmi_hw {
        type hw
        =HDMICARD=
        =HDMIDEVICE=
        channels 2
}

pcm.hdmi_formatted {
        type plug
        slave {
                pcm hdmi_hw
                rate 48000
                channels 2
        }
}

pcm.hdmi_complete {
        type softvol
        slave.pcm hdmi_formatted
        control.name hdmi_volume
        control.=HDMICARD=
}

pcm.digital_hw {
        type hw
        =DIGITALCARD=
        =DIGITALDEVICE=
        channels 2
}

pcm.analog_hw {
        type hw
        =ANALOGCARD=
        =ANALOGDEVICE=
        channels 2
}
EOF

        sed -i "s/=HDMICARD=/card $HDMICARD/g" /home/$xbmcUser/.asoundrc
        sed -i "s/=HDMIDEVICE=/device $HDMIDEVICE/g" /home/$xbmcUser/.asoundrc

        sed -i "s/=DIGITALCARD=/card $DIGITALCARD/g" /home/$xbmcUser/.asoundrc
        sed -i "s/=DIGITALDEVICE=/device $DIGITALDEVICE/g" /home/$xbmcUser/.asoundrc

        sed -i "s/=ANALOGCARD=/card $ANALOGCARD/g" /home/$xbmcUser/.asoundrc
        sed -i "s/=ANALOGDEVICE=/device $ANALOGDEVICE/g" /home/$xbmcUser/.asoundrc

        chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.asoundrc

	restartALSA="1"
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

# Remove entry in rc.local
sed -i -e "/XBMCLIVE/d" /etc/rc.local 
BLOB

chmod +x /usr/bin/hwSetupNvidiaHDMI.sh

sed -i -e "/exit 0/d" /etc/rc.local 

echo "# XBMCLIVE" >> /etc/rc.local 
echo "/usr/bin/hwSetupNvidiaHDMI.sh # XBMCLIVE" >> /etc/rc.local 
echo "# XBMCLIVE" >> /etc/rc.local 
echo "exit 0" >> /etc/rc.local 
