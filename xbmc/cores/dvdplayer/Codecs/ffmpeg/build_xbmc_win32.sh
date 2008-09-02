#!/bin/bash 
rm -r .libs

./configure \
--extra-cflags="-D_XBOX -mno-cygwin -mms-bitfields" \
--extra-ldflags="-Wl,--add-stdcall-alias" \
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-swscale \
--enable-protocol=http \
--disable-static \
--disable-altivec \
--disable-vhook \
--disable-ffserver \
--disable-ffmpeg \
--disable-ffplay \
--disable-muxers \
--disable-encoders \
--disable-ipv6 \
--disable-debug \
--target-os=mingw32 && 
 
make -j3 && 
strip lib*/*.dll &&
mkdir .libs &&
cp lib*/*.dll .libs/