#!/bin/sh

# If building on OSX 10.4, you will need to update make from 3.80 to 3.81. 
# Use --prefix=/usr for the configure and since XCode builds have a path
# with "/Developer/usr/bin" need to make/install again using 
# --prefix=/Developer/usr/bin
#
# The postfix name "-osx" must match the postfix name defined in the XCode run script
# call "configure".
#
if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

make distclean >/dev/null 2>&1 

export MACOSX_DEPLOYMENT_TARGET=10.4

echo "Configuring ffmpeg"
./configure \
    --extra-cflags="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -I/opt/local/include -L/opt/local/lib -D_XBOX -DRUNTIME_CPUDETECT -O3" \
    --extra-ldflags="-L/opt/local/lib" \
    --enable-static \
    --disable-altivec \
    --disable-vhook \
    --disable-debug \
    --disable-muxers \
    --disable-encoders \
    --disable-devices \
    --disable-ffplay \
    --disable-ffserver \
    --disable-ffmpeg \
    --disable-shared \
    --disable-ipv6 \
    --enable-postproc \
    --enable-gpl \
    --enable-swscale \
    --enable-protocol=http \
    --enable-pthreads \
    --enable-libvorbis

echo "Building ffmpeg"
make clean
make

echo wrapping libavutil
ar rus avutil-49-osx.a libavutil/*.o 

echo wrapping libavcodec
libtool -static -o avcodec-52-osx.a libavcodec/*.o libavcodec/x86/*.o /opt/local/lib/libvorbis.a /opt/local/lib/libvorbisenc.a

echo wrapping libavformat
ar rus avformat-52-osx.a libavformat/*.o

echo wrapping libswscale
ar rus swscale-0.6.1-osx.a libswscale/*.o

echo wrapping libpostproc
ar rus postproc-51-osx.a libpostproc/*.o

echo copying libs
cp -v avcodec-52-osx.a avformat-52-osx.a avutil-49-osx.a swscale-0.6.1-osx.a postproc-51-osx.a "$XBMC_ROOT"/system/players/dvdplayer

# distclean after making
make distclean >/dev/null 2>&1 
