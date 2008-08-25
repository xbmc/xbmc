#!/bin/bash  

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

cd $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libDVDCSS/
make distclean >/dev/null 2>&1

./configure CFLAGS="-fPIC -DXBMC -D_DLL -D_LINUX -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" \
    --with-pic

make

cd $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libdvdnav/
make distclean >/dev/null 2>&1

./configure CFLAGS="-D_XBOX -DHAVE_DVDCSS_DVDCSS_H -fPIC -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" \
    --with-pic

make

echo wrapping libdvdnav

gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o libdvdnav-osx.so src/.libs/*.o src/dvdread/.libs/*.o src/vm/.libs/*.o ../libDVDCSS/src/*.o

cd $XBMC_ROOT/tools/Mach5;./mach5.rb $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libdvdnav/libdvdnav-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/libdvdnav-osx.so

rm $XBMC_ROOT/xbmc/cores/dvdplayer/Codecs/libdvdnav/libdvdnav-osx.so
