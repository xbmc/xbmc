#!/bin/bash

# $0 is this script's path
# $1 would be the package's path
# $2 would be the install path
# $3 target volume

# Copy default sources if it doesn't exist
mkdir -p "$HOME/Library/Application Support/XBMC/userdata"
if [ ! -f  "$HOME/Library/Application Support/XBMC/userdata/sources.xml" ] ; then
    cp "/var/tmp/sources.xml" "$HOME/Library/Application Support/XBMC/userdata"
fi

#make sure owner of the app is the user and not root
/usr/sbin/chown -f -R $USER $2/XBMC.app

