#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

echo wrapping liba52
gcc -shared -fpic --soname,liba52-i486-linux.so -o liba52-i486-linux.so -rdynamic liba52/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo wrapping libao
gcc -shared -fpic --soname,libao-i486-linux.so -o libao-i486-linux.so -rdynamic libao/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo copying lib
cp -v liba52-i486-linux.so libao-i486-linux.so $XBMC_ROOT/system/players/dvdplayer


