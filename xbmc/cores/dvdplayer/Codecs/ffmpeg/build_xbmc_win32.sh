#!/bin/bash 
rm -r .libs
make distclean

./configure \
--extra-cflags="-fno-common -I../libfaad2/include -Iinclude/dxva2" \
--extra-ldflags="-L../../../../../system/players/dvdplayer" \
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-zlib \
--enable-libfaad \
--disable-static \
--disable-altivec \
--disable-muxers \
--disable-encoders \
--disable-debug \
--enable-muxer=spdif \
--enable-muxer=adts \
--enable-encoder=ac3 \
--enable-encoder=aac &&
 
make -j3 && 
mkdir .libs &&
cp lib*/*.dll .libs/ &&
mv .libs/swscale-0.dll .libs/swscale-0.6.1.dll &&
cp .libs/avcodec-52.dll ../../../../../system/players/dvdplayer/ &&
cp .libs/avformat-52.dll ../../../../../system/players/dvdplayer/ &&
cp .libs/avutil-50.dll ../../../../../system/players/dvdplayer/ &&
cp .libs/postproc-51.dll ../../../../../system/players/dvdplayer/ &&
cp .libs/swscale-0.6.1.dll ../../../../../system/players/dvdplayer/ &&
cp libavutil/avconfig.h include/libavutil/

