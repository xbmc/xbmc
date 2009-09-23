#!/bin/bash 

make distclean >/dev/null 2>&1 

./configure \
 --with-pic \
  --disable-static \
  --enable-shared \
  --disable-directx \
  --disable-sdl \
  --without-x 

make

strip libmpeg2/.libs/*.dll

