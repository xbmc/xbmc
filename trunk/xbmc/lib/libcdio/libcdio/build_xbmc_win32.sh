#!/bin/bash 

#export LDFLAGS="-Wl,--add-stdcall-alias"
#export CPPFLAGS="-I../"

./configure \
--enable-static \
--disable-shared \
--without-cd-drive \
--without-cd-info \
--without-cdda-player \
--without-cd-read \
--without-iso-info \
--without-iso-read \
--without-cd-paranoia && 
 
make -j3
cp lib/driver/.libs/libcdio.a ./libcdio_win32.lib