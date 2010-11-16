#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

./configure --extra-cflags="--enable-shared --enable-postproc --enable-gpl --disable-static --disable-vhook --enable-swscale --enable-protocol=http --disable-altivec --disable-ipv6 --enable-pthreads --disable-debug"

make

echo wrapping libavutil
gcc -shared -fPIC --soname,avutil-51-i486-linux.so -o avutil-51-i486-linux.so -rdynamic libavutil/*.o 

echo wrapping libavcodec
gcc -o avcodec-51-i486-linux.so --soname,avcodec-51-i486-linux.so -shared -fPIC -rdynamic libavcodec/*.o libavcodec/i386/*.o `cat $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.def` $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.o

echo wrapping libavformat
gcc -o avformat-51-i486-linux.so --soname,avformat-51-i486-linux.so -shared  -fPIC  -rdynamic libavformat/*.o `cat $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.def`,-export-all-symbols $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.o

echo wrapping libswscale
gcc -o swscale-51-i486-linux.so --soname,swscale-51-i486-linux.so -shared -fPIC -rdynamic libswscale/*.o `cat $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.def` $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.o

echo wrapping libpostproc
gcc -o postproc-51-i486-linux.so --soname,postproc-51-i486-linux.so -shared -fPIC -rdynamic libpostproc/*.o `cat $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.def` $XBMC_ROOT/xbmc/cores/DllLoader/exports/wrapper.o

echo copying libs
cp -v avcodec-51-i486-linux.so avformat-51-i486-linux.so avutil-51-i486-linux.so swscale-51-i486-linux.so postproc-51-i486-linux.so $XBMC_ROOT/system/players/dvdplayer


