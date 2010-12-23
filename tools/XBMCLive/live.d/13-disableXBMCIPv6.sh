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

activationToken="doNotDisableIPv6InXBMC"

# if strings are the same the token is NOT part of the parameters list
# here we want to stop script if the token is NOT there
if [ "$xbmcParams" != "${xbmcParams%$activationToken*}" ] ; then
	exit
fi

xbmcUser=xbmc
# Read configuration variable file if it is present
[ -r /etc/default/xbmc-live ] && . /etc/default/xbmc-live
if ! getent passwd $xbmcUser >/dev/null; then
	xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')
fi

if [ ! -f /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml ] ; then
	mkdir -p /home/$xbmcUser/.xbmc/userdata &> /dev/null
	cat > /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml << 'EOF'
<advancedsettings>
  <network>
    <disableipv6>true</disableipv6>
  </network>
</advancedsettings>
EOF
	chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.xbmc
else
	if ! grep -i -q disableipv6 /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml ; then
		if grep -i -q "<network>" /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml ; then
			sed -i -e 's%<network>%&\n<disableipv6>true</disableipv6>%' /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml
		else
			sed -i -e 's%<advancedsettings>%&\n<network>\n<disableipv6>true</disableipv6>\n</network>%' /home/$xbmcUser/.xbmc/userdata/advancedsettings.xml
		fi
	fi
fi

exit 0
