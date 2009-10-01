#!/bin/bash 
# Requires libtool

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

PLATFORM=`uname -m`
if [ $PLATFORM != "x86_64" ]; then
   PLATFORM="i486"
fi

make distclean

autoreconf -vif &&
./configure --with-mp4v2 --with-pic &&
make &&

echo wrapping libfaad &&
gcc -shared -fpic --soname,libfaad-$PLATFORM-linux.so -o libfaad-$PLATFORM-linux.so -rdynamic libfaad/*.o `cat $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.def` &&

echo copying lib &&
cp -v libfaad-$PLATFORM-linux.so $XBMC_ROOT/system/players/dvdplayer


