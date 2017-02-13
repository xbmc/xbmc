#!/bin/sh

export OETOP=/mnt/storage/openspa5/builds/openspa/vusolo2

export OEBIN="$OETOP/tmp/sysroots/x86_64-linux/usr/bin/mipsel-oe-linux:$OETOP/tmp/sysroots/x86_64-linux/usr/bin"
export OESYSROOT="$OETOP/tmp/sysroots/vusolo2"

export PATH=$OEBIN:$PATH

GCC_PREFIX="mipsel-oe-linux-"
GCC_EXTCFLAGS="-mel -mabi=32 -mhard-float -march=mips32 --sysroot=$OESYSROOT"
GCC_EXTLDFLAGS="--sysroot=$OESYSROOT"

CFLAGS="-O2 -g -D_DEBUG -Wall  -fPIC -DPIC -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"
CXXFLAGS="$CFLAGS -std=gnu++11"

CC="${GCC_PREFIX}gcc $GCC_EXTCFLAGS" 
CXX="${GCC_PREFIX}g++ $GCC_EXTCFLAGS" 
LD="${GCC_PREFIX}ld $GCC_EXTLDFLAGS" 
AR="${GCC_PREFIX}ar"

CC="$CC" CXX="$CXX" LD="$LD" AR="$AR" CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" ./autobuild_vuplus_mips.sh

