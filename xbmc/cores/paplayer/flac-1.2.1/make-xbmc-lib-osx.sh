#!/bin/bash  

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean >/dev/null 2>&1

./configure -C \
    --with-pic \
    --disable-asm-optimizations \
    --disable-xmms-plugin \
    --disable-cpplibs \
    CFLAGS="-D_LINUX -D_DLL -fPIC -O2 -fno-common -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"

sed -ie s/__DECLARE__XBMC__ARCH__/osx/ Makefile


# make handles the wrapping
make

# distclean after making
#make distclean >/dev/null 2>&1

