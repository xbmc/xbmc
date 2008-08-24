#!/bin/bash 
# Requires libtool

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean

autoreconf -vif &&
./configure --with-mp4v2 --with-pic CFLAGS="-fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" &&
make &&

echo wrapping libfaad &&

gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o libfaad-osx.so libfaad/*.o 

cd $XBMC_ROOT/tools/Mach5;./mach5.rb $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libfaad2/libfaad-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/libfaad-osx.so

rm $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libfaad2/libfaad-osx.so


