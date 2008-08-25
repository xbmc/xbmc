#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean

./configure CFLAGS="-fPIC -DXBMC -D_DLL -D_LINUX -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4" \
    --with-pic

make

echo wrapping libmpeg2
gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o libmpeg2-osx.so libmpeg2/*.o
mv libmpeg2-osx.so $XBMC_ROOT/tools/Mach5/

cd $XBMC_ROOT/tools/Mach5
./mach5.rb libmpeg2-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/libmpeg2-osx.so; rm libmpeg2-osx.so


