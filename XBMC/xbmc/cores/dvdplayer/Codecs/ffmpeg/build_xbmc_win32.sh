#!/bin/bash 
rm -r .libs
make distclean

./configure \
--extra-cflags="-D_XBOX -fno-common" \
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
--disable-debug \
--enable-encoder=ac3 && 
 
make -j3 && 
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll
