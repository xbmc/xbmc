#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

echo wrapping libfaad
gcc -shared -fpic --soname,libfaad-i486-linux.so -o libfaad-i486-linux.so -rdynamic libfaad/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo copying lib
cp -v libfaad-i486-linux.so $XBMC_ROOT/system/players/dvdplayer


