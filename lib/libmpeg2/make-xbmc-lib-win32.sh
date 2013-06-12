#!/bin/bash

MAKEFLAGS=""
BGPROCESSFILE="$2"

if [ "$1" == "clean" ]
then
  if [ -d libmpeg2/.libs ]
  then
    rm -r libmpeg2/.libs
  fi
  make distclean
fi

if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  MAKEFLAGS=-j$NUMBER_OF_PROCESSORS
fi

./configure \
 --with-pic \
  --disable-static \
  --enable-shared \
  --disable-directx \
  --disable-sdl \
  --without-x &&

make $MAKEFLAGS &&

strip libmpeg2/.libs/*.dll &&
cp libmpeg2/.libs/*.dll /xbmc/system/players/dvdplayer/

#remove the bgprocessfile for signaling the process end
rm $BGPROCESSFILE
