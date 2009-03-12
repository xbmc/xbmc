#!/bin/bash

if [ "$(pidof X)" ] ; then
	/usr/share/xbmc/xbmc.bin  -q --standalone
	exit
fi

while true
do
	echo "#!/bin/bash" >  /home/xbmc/.xsession
	echo "/usr/share/xbmc/xbmc.bin  -q --standalone" >>  /home/xbmc/.xsession

	echo "case \"\$?\" in" >>  /home/xbmc/.xsession
	echo "    0 ) # Quit" >>  /home/xbmc/.xsession
	echo "        touch /tmp/noRestartXBMC" >> /home/xbmc/.xsession
	echo "        break ;;"  >>  /home/xbmc/.xsession
	echo "    64 ) # Shutdown System"  >>  /home/xbmc/.xsession
	echo "        sleep 10 ;;"  >>  /home/xbmc/.xsession
	echo "    65 ) # Warm Reboot"  >>  /home/xbmc/.xsession
	echo "        echo "Restarting XBMC ..." ;;"  >>  /home/xbmc/.xsession
	echo "    66 ) # Reboot System"  >>  /home/xbmc/.xsession
	echo "        sleep 10 ;;"  >>  /home/xbmc/.xsession
	echo "     * ) ;;"  >>  /home/xbmc/.xsession
	echo "esac"  >>  /home/xbmc/.xsession

	chown xbmc:xbmc /home/xbmc/.xsession

	if [ "$(whoami)" == "root" ] ; then
		su xbmc -c "startx -- -br > /dev/null 2>&1" -l
	else
		startx -- -br > /dev/null 2>&1
	fi

	if [ -e /tmp/noRestartXBMC ] ; then
		rm /tmp/noRestartXBMC
		rm /home/xbmc/.xsession
		break
	fi

#	sleep 2
done
