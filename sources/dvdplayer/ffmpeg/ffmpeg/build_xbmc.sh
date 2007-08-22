#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

./configure --extra-cflags=-D_XBOX --enable-shared --enable-pp --enable-memalign-hack --enable-gpl --disable-static --disable-vhook --disable-encoders --enable-swscaler

make

echo wrapping libavutil
gcc -shared -fpic --soname,avutil-51-i486-linux.so -o avutil-51-i486-linux.so -rdynamic libavutil/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo wrapping libavcodec
gcc -o avcodec-51-i486-linux.so --soname,avcodec-51-i486-linux.so -shared -fpic -rdynamic libavcodec/*.o libavcodec/i386/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'` 

echo wrapping libavformat
gcc -o avformat-51-i486-linux.so --soname,avformat-51-i486-linux.so -shared  -fpic  -rdynamic libavformat/*.o -Wl,-export-all-symbols`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo wrapping libswscale
gcc -o swscale-51-i486-linux.so --soname,swscale-51-i486-linux.so -shared -fpic -rdynamic libswscale/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo wrapping libpostproc
gcc -o postproc-51-i486-linux.so --soname,postproc-51-i486-linux.so -shared -fpic -rdynamic libpostproc/*.o -Wl`sed 's/[(\*]/ /g' $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.c | grep -v bash | awk '/__wrap/ {out=out","$2} END {print out}' | sed 's/__wrap_/-wrap,/g'`

echo copying libs
cp -v avcodec-51-i486-linux.so avformat-51-i486-linux.so avutil-51-i486-linux.so swscale-51-i486-linux.so postproc-51-i486-linux.so $XBMC_ROOT/system/players/dvdplayer
cp -v avcodec-51-i486-linux.so avformat-51-i486-linux.so avutil-51-i486-linux.so swscale-51-i486-linux.so postproc-51-i486-linux.so $XBMC_HOME/system/players/dvdplayer


