#!/bin/sh

#
#some q&d to avoid editing system config files.
#
depmod -a `uname -r`
modprobe lcd
modprobe hdmi
modprobe ump
modprobe disp
modprobe mali
modprobe mali_drm
chmod 666 /dev/mali /dev/ump /dev/cedar_dev /dev/disp
chmod -R 666 /dev/input/*
chmod -R 666 /dev/snd/*

stop lightdm

#thanks, Sam Nazarko 

while true
do
    su - miniand -c "/allwinner/xbmc-pvr-bin/lib/xbmc/xbmc.bin --standalone -fs --lircdev /var/run/lirc/lircd 2>&1 | logger -t xbmc"
    case "$?" in
         0) # user quit. 
	    	sleep 2 ;;
        64) # shutdown system.
            poweroff;;
        65) # warm Restart xbmc
            sleep 2 ;;
        66) # Reboot System
            reboot;;
         *) # this should not happen
            sleep 30 ;;
    esac
done
