#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean >/dev/null 2>&1 

./configure CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"

make

ar rs libcdio-osx.a  lib/driver/.libs/*.o
mv libcdio-osx.a ../

# distclean after making
make distclean >/dev/null 2>&1 

