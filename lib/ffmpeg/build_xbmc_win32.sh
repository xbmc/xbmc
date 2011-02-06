#!/bin/bash

if [ -d .libs ]
then
rm -r .libs
fi

if [ -f config.mak ]
then
make distclean
fi

OPTIONS="
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-zlib \
--disable-static \
--disable-altivec \
--disable-muxers \
--disable-encoders \
--disable-debug \
--disable-ffplay \
--disable-ffserver \
--disable-ffmpeg \
--disable-ffprobe \
--disable-devices \
--enable-muxer=spdif \
--enable-muxer=adts \
--enable-encoder=ac3 \
--enable-encoder=aac \
--enable-runtime-cpudetect \
--disable-debug"

./configure --extra-cflags="-fno-common -Iinclude-xbmc-win32/dxva2" --extra-ldflags="-L/xbmc/system/players/dvdplayer" ${OPTIONS} &&
 
make -j3 && 
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll &&
cp .libs/avcodec-52.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avcore-0.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avformat-52.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avutil-50.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/postproc-51.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/swscale-0.6.1.dll /xbmc/system/players/dvdplayer/
