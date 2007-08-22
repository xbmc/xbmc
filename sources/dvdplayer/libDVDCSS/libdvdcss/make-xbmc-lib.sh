#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

echo wrapping libDVDCSS
gcc -shared -fpic --soname,libDVDCSS-i486-linux.so -o libDVDCSS-i486-linux.so -rdynamic src/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo copying lib
cp -v libDVDCSS-i486-linux.so $XBMC_ROOT/system/players/dvdplayer


