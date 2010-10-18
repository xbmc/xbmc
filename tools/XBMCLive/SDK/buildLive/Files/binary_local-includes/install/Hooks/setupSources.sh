#!/bin/sh

xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')

#
# Create directories for XBMC sources
#

if [ ! -d "/home/$xbmcUser/Music" ]; then
	mkdir /home/$xbmcUser/Music
	chmod 755 /home/$xbmcUser/Music
	chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/Music
fi

if [ ! -d "/home/$xbmcUser/Pictures" ]; then
	mkdir /home/$xbmcUser/Pictures
	chmod 755 /home/$xbmcUser/Pictures
	chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/Pictures
fi

if [ ! -d "/home/$xbmcUser/TV Shows" ]; then
	mkdir "/home/$xbmcUser/TV Shows"
	chmod 755 "/home/$xbmcUser/TV Shows"
	chown -R $xbmcUser:$xbmcUser "/home/$xbmcUser/TV Shows"
fi

if [ ! -d "/home/$xbmcUser/Videos" ]; then
	mkdir /home/$xbmcUser/Videos
	chmod 755 /home/$xbmcUser/Videos
	chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/Videos
fi

mkdir -p /home/$xbmcUser/.xbmc/userdata &> /dev/null

if [ ! -f /home/$xbmcUser/.xbmc/userdata/sources.xml ] ; then
	cat > /home/$xbmcUser/.xbmc/userdata/sources.xml << EOF
<sources>
    <video>
        <default pathversion="1"></default>
        <source>
            <name>Videos</name>
            <path pathversion="1">/home/$xbmcUser/Videos/</path>
        </source>
        <source>
            <name>TV Shows</name>
            <path pathversion="1">/home/$xbmcUser/TV Shows/</path>
        </source>
    </video>
    <music>
        <default pathversion="1"></default>
        <source>
            <name>Music</name>
            <path pathversion="1">/home/$xbmcUser/Music/</path>
        </source>
    </music>
    <pictures>
        <default pathversion="1"></default>
        <source>
            <name>Pictures</name>
            <path pathversion="1">/home/$xbmcUser/Pictures/</path>
        </source>
    </pictures>
</sources>
EOF
fi
chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.xbmc
