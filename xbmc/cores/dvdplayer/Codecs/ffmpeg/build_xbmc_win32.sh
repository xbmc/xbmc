#!/bin/bash 
if [ -d .libs ]
then
rm -r .libs
fi

make distclean

OPTIONS="
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
--enable-encoder=aac"

if [ -d ../libvpx ]
then
echo Building libvpx ...
echo
cd ../libvpx
./configure --disable-examples --enable-vp8 --target=x86-win32-gcc && make
cd ../ffmpeg
fi

if [ -f ../libvpx/libvpx.a ] 
then
echo Building ffmpeg with libvpx ...
echo
OPTIONS="$OPTIONS --enable-libvpx"
fi

./configure --extra-cflags="-fno-common -I../libfaad2/include -Iinclude/dxva2 -I../libvpx/" --extra-ldflags="-L../../../../../system/players/dvdplayer -L../libvpx" ${OPTIONS} &&
 
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

