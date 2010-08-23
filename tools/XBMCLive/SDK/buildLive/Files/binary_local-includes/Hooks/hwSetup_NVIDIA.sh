#!/bin/bash


#check Nvidia GPU
nvidiaGpuType=lspci -nn | grep '03:00.0' | grep 'nVidia Corporation'
if [ ! -n "$nvidiaGpuType" ] ; then
	exit 0
fi

xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')

mkdir -p /home/$xbmcUser/.xbmc/userdata

if ! -f /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml ; then
	cat > /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml << EOF
<advancedsettings>
    <gputempcommand>echo "$(nvclock -T | sed -ne "s/=> GPU temp.*: \([0-9]\+\).*/\1/p") C"</gputempcommand>
    <useddsfanart>true</useddsfanart>
</advancedsettings>
EOF
fi

chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.xbmc