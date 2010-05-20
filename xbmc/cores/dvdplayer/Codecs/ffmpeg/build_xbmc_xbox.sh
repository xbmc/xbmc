#!/bin/bash 
rm -r .libs
make distclean

./configure \
--extra-cflags="-D_XBOX -fno-common" \
--cpu=pentium3 \
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-swscale \
--enable-zlib \
--disable-static \
--disable-altivec \
--disable-vhook \
--disable-muxers \
--disable-encoders \
--disable-ipv6 \
--disable-debug && 
 
make -j3 && 
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll
cp .libs/avcodec-52.dll ../../../../../system/players/dvdplayer/
cp .libs/avformat-52.dll ../../../../../system/players/dvdplayer/
cp .libs/avutil-49.dll ../../../../../system/players/dvdplayer/
cp .libs/postproc-51.dll ../../../../../system/players/dvdplayer/
cp .libs/swscale-0.6.1.dll ../../../../../system/players/dvdplayer/