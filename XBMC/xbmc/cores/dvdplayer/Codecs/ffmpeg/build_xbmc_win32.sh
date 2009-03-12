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
--enable-protocol=http \
--enable-encoders \
--disable-static \
--disable-altivec \
--disable-vhook \
--enable-muxers \
--disable-ipv6 \
--disable-debug && 
 
make -j3 && 
strip lib*/*.dll &&
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll
make install
