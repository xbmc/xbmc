#!/bin/bash

#check AMD GPU
amdGpuType=$(lspci -nn | grep 'VGA' | grep 1002)
if [ ! -n "$amdGpuType" ] ; then
	exit 0
fi

xbmcUser=$(getent passwd 1000 | sed -e 's/\:.*//')

mkdir -p /home/$xbmcUser/.xbmc/userdata

#
# Always sync to vblank
#
if [ ! -f /home/$xbmcUser/.xbmc/userdata/guisettings.xml ] ; then
	cat > /home/$xbmcUser/.xbmc/userdata/guisettings.xml << 'EOF'
<settings>
    <videoscreen>
        <vsync>2</vsync>
    </videoscreen>
</settings>
EOF
fi

chown -R $xbmcUser:$xbmcUser /home/$xbmcUser/.xbmc
