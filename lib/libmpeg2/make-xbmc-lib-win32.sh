#!/bin/bash 

if [ -d libmpeg2/.libs ]
then
rm -r libmpeg2/.libs
fi

if [ -f config.log ]
then
make distclean
fi

./configure \
 --with-pic \
  --disable-static \
  --enable-shared \
  --disable-directx \
  --disable-sdl \
  --without-x &&

make &&

strip libmpeg2/.libs/*.dll &&
cp libmpeg2/.libs/*.dll /xbmc/system/players/dvdplayer/
