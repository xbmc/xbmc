#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean

./configure CFLAGS="-fPIC -DXBMC -D_DLL -D_LINUX -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"

make

gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o liba52-osx.so liba52/*.o
mv liba52-osx.so $XBMC_ROOT/tools/Mach5/
gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o libao-osx.so libao/*.o
mv libao-osx.so $XBMC_ROOT/tools/Mach5/

echo wrapping liba52
cd $XBMC_ROOT/tools/Mach5

./mach5.rb liba52-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/liba52-osx.so; rm liba52-osx.so

echo wrapping libao
./mach5.rb libao-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/libao-osx.so; rm libao-osx.so

