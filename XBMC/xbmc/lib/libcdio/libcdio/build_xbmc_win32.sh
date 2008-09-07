#!/bin/bash 

export LDFLAGS="-Wl,--add-stdcall-alias"
export CPPFLAGS="-I../"

./configure \
--enable-static \
--disable-shared \
--without-cd-drive \
--without-cd-info \
--without-cdda-player \
--without-cd-read \
--without-iso-info \
--without-iso-read \
--enable-memalign-hack && 
 
make -j3 && 
strip lib/driver/.libs/*.dll