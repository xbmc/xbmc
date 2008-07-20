#!/bin/sh

# Copy default sources if it doesn't exist
mkdir -p "$HOME/Library/Application Support/XBMC/userdata"
if [ ! -f  "$HOME/Library/Application Support/XBMC/userdata/sources.xml" ] ; then
    cp "/var/tmp/sources.xml" "$HOME/Library/Application Support/XBMC/userdata"
fi
