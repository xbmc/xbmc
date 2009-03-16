#!/bin/bash 

if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

# Get ARCH from Makefile.include
file=../../../../Makefile.include
if [ -f $file ]
then
  ARCH=$(grep "^ARCH=" $file | awk -F"=" '{print $2}')
else
  echo "$file not found... You must run configure first!"
  exit 1
fi


export MACOSX_DEPLOYMENT_TARGET=10.4

make distclean >/dev/null 2>&1 

./configure CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4"

make

ar rs libcdio-$ARCH.a  lib/driver/.libs/*.o
mv libcdio-$ARCH.a ../

# Do not distclean after making (it removes the Makefile)
#make distclean >/dev/null 2>&1 

