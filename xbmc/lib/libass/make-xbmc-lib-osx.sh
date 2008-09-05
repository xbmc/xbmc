#!/bin/bash  

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean >/dev/null 2>&1

./configure CFLAGS="-D_LINUX -D_DLL -fPIC -O2 -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"

make

echo wrapping libass

gcc -bundle -flat_namespace -undefined suppress -shared -fPIC \
	-mmacosx-version-min=10.4 -o libass-osx.so libass/.libs/*.o

cd $XBMC_ROOT/tools/Mach5;./mach5.rb $XBMC_ROOT/xbmc/lib/libass/libass-osx.so;mv output.so $XBMC_ROOT/system/players/dvdplayer/libass-osx.so

rm $XBMC_ROOT/xbmc/lib/libass/libass-osx.so

# distclean after making
cd $XBMC_ROOT/xbmc/lib/libass/
make distclean >/dev/null 2>&1

