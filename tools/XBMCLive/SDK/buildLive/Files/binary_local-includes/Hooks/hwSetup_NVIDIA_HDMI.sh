#!/bin/bash
NvidiaHDMI=$(lspci -nn | grep 'Audio' | grep '10de:0be3')

if [ ! -n "$NvidiaHDMI" ] ; then
        exit 0
fi

if ! grep -i -q snd-hda-intel /etc/modprobe.d/alsa-base.conf ; then
	CARD=`aplay -l | grep 'NVIDIA HDMI' | awk -F: '{ print $1 }' | awk '{ print $2 }'`
	DEVICE=`aplay -l | grep 'NVIDIA HDMI' | awk -F: '{ print $2 }' | awk '{ print $5 }'`

	if [ $CARD,$DEVICE == 1,3 ]; then
		echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
	elif [ $CARD,$DEVICE == 0,3 ]; then
		echo 'options snd-hda-intel enable_msi=0 probe_mask=0xfff2' >> /etc/modprobe.d/alsa-base.conf
	elif [ $CARD,$DEVICE == 2,3 ]; then
		echo 'options snd-hda-intel enable_msi=0 probe_mask=0xffff,0xffff,0xfff2' >> /etc/modprobe.d/alsa-base.conf
#	else
#		echo "No matching value detected"
	fi
fi
