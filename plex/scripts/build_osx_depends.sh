#!/bin/sh

ROOT=$(pwd)
DEPENDDIR=$ROOT/tools/darwin/depends
DEPEND_HASH=$(git rev-list -1 HEAD -- $DEPENDDIR | cut -c1-8)
FFMPEG_HASH=$(git rev-list -1 HEAD -- $ROOT/lib/ffmpeg | cut -c1-8)

target_os=$1

if [ -z $target_os ]; then
  target_os="osx"
fi

if [ $target_os = "ios" ]; then
  darwin="ios"
  arch="armv7"
elif [ $target_os = "osx" ]; then
  darwin="osx"
  arch="i386"
elif [ $target_os = "osx64" ]; then
  darwin="osx"
  arch="x86_64"
fi

echo "Building for $darwin-$arch"

xcodepath=$(xcode-select -print-path)
xcodebuild=$xcodepath/usr/bin/xcodebuild
if [ $darwin = "osx" ]; then
  sdkversion=$($xcodebuild -showsdks | grep macosx | sort |  tail -n 1 | grep -oE 'macosx[0-9.0-9]+' | cut -c 7-$NF)
else
  sdkversion=$($xcodebuild -showsdks | grep iphoneos | sort | tail -n 1 | awk '{ print $2}')
fi

echo "SDK $sdkversion"
if [ $darwin = "osx" ]; then
  outputdir=macosx${sdkversion}_$arch
else
  outputdir=iphoneos${sdkversion}_$arch
fi
outputpath=$ROOT/plex/Dependencies/xbmc-depends/$outputdir

echo $outputpath
cd $DEPENDDIR
./bootstrap
./configure --with-rootpath=$ROOT/plex/Dependencies/xbmc-depends --with-toolchain=/Users/Shared/xbmc-depends/toolchain --with-darwin=$darwin --with-arch=$arch
make || exit 1

cd $ROOT
cd $ROOT/lib/ffmpeg
config="--target-os=darwin --disable-muxers --disable-encoders --disable-devices --disable-doc --disable-ffplay --disable-ffmpeg"
config="$config --disable-ffprobe --disable-ffserver --disable-vda --disable-crystalhd --disable-decoder=mpeg_xvmc --disable-debug"
if [ $arch = "arm" ]; then
  config="$config --arch=$arch --enable-cross-compile --enable-pic --disable-armv5te --disable-armv6t2 --enable-neon"
elif [ $arch = "i386" ]; then
  config="$config --arch=x86 --enable-cross-compile --disable-amd3dnow"
else
  config="$config --disable-amd3dnow"
fi

config="$config --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis"
config="$config --enable-gpl --enable-postproc --enable-static --enable-pthreads"
config="$config --enable-muxer=spdif --enable-muxer=adts --enable-encoder=ac3 --enable-encoder=aac"
config="$config --enable-protocol=http --enable-runtime-cpudetect"
config="$config --cc=clang --prefix=$ROOT/plex/Dependencies/xbmc-depends/ffmpeg-$outputdir"

export PATH=/Users/Shared/xbmc-depends/toolchain/bin:$PATH
./configure $config --extra-cflags="-arch $arch -I$outputpath/include" --extra-ldflags="-arch $arch -L$outputpath/lib" || exit 1
make install || exit 1

cd $ROOT/plex/Dependencies/xbmc-depends
rm -f $ROOT/plex/Dependencies/*tar.xz

echo "Packing xbmc-depends"
gtar --xz -cf $ROOT/plex/Dependencies/$outputdir-xbmc-$DEPEND_HASH.tar.xz $outputdir

echo "Packing ffmpeg"
gtar --xz -cf $ROOT/plex/Dependencies/$outputdir-ffmpeg-$FFMPEG_HASH.tar.xz ffmpeg-$outputdir
