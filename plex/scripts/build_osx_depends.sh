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
  sdkversion=10.8
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
./configure --with-sdk=10.8 --with-rootpath=$ROOT/plex/Dependencies/xbmc-depends --with-toolchain=/Users/Shared/xbmc-depends/toolchain --with-darwin=$darwin --with-arch=$arch
make || exit 1

cd $ROOT

# read some variables
source tools/darwin/depends/config.site

# ./configure --extra-cflags="-O2  -std=gnu99 -no-cpp-precomp -miphoneos-version-min=4.2 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk -arch armv7 -mcpu=cortex-a8 -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -pipe -Wno-trigraphs -fpascal-strings -O3 -Wreturn-type -Wunused-variable -fmessage-length=0 -gdwarf-2 -I/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk/usr/include  -g -D_DEBUG -Wall  -w -D_DARWIN_C_SOURCE -Dattribute_deprecated=" --target-os=darwin --disable-muxers   --disable-encoders --disable-devices  --disable-doc --disable-ffplay   --disable-ffmpeg --disable-ffprobe  --disable-ffserver --disable-vda      --disable-crystalhd --disable-decoder=mpeg_xvmc --cpu=cortex-a8 --arch=arm --enable-cross-compile --enable-pic --disable-armv5te --disable-armv6t2 --enable-neon --disable-libvorbis --enable-gpl --enable-postproc --enable-static      --enable-pthreads --enable-muxer=spdif --enable-muxer=adts --enable-encoder=ac3 --enable-encoder=aac --enable-protocol=http --enable-runtime-cpudetect --cc=clang --as="/Users/Shared/xbmc-depends/toolchain/bin/gas-preprocessor.pl /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/llvm-gcc-4.2"

# ./configure --target-os=darwin --disable-muxers --disable-encoders --disable-devices --disable-doc --disable-ffplay --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-vda --disable-crystalhd --disable-decoder=mpeg_xvmc --disable-debug --cpu=cortex-a8 --arch=arm --enable-cross-compile --enable-pic --disable-armv5te --disable-armv6t2 --enable-neon --disable-libvorbis --enable-gpl --enable-postproc --enable-static --enable-pthreads --enable-muxer=spdif --enable-muxer=adts --enable-encoder=ac3 --enable-encoder=aac --enable-protocol=http --enable-runtime-cpudetect --prefix=/Users/tru/Documents/Code/Plex/plex-home-theater/plex/Dependencies/xbmc-depends/ffmpeg-iphoneos6.1_armv7 --cc=clang --as=/Users/Shared/xbmc-depends/toolchain/bin/gas-preprocessor.pl /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/llvm-gcc-4.2 --extra-cflags=-mfpu=neon  -std=gnu99 -no-cpp-precomp -miphoneos-version-min=4.2 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk -arch armv7 -mcpu=cortex-a8 -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -pipe -Wno-trigraphs -fpascal-strings -O3 -Wreturn-type -Wunused-variable -fmessage-length=0 -gdwarf-2 -I/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk/usr/include -I/Users/tru/Documents/Code/Plex/plex-home-theater/plex/Dependencies/xbmc-depends/iphoneos6.1_armv7/include --extra-ldflags=-arch armv7 -L/Users/tru/Documents/Code/Plex/plex-home-theater/plex/Dependencies/xbmc-depends/iphoneos6.1_armv7/lib

cd $ROOT/lib/ffmpeg
config="--target-os=darwin --disable-muxers --disable-encoders --disable-devices --disable-doc --disable-ffplay --disable-ffmpeg"
config="$config --disable-ffprobe --disable-ffserver --disable-vda --disable-crystalhd --disable-decoder=mpeg_xvmc --disable-debug"
if [ $arch = "armv7" ]; then
  config="$config --cpu=cortex-a8 --arch=arm --enable-cross-compile --enable-pic --disable-armv5te --disable-armv6t2 --enable-neon"
  extra_cflags="-mfpu=neon $CFLAGS"
elif [ $arch = "i386" ]; then
  config="$config --arch=x86 --enable-cross-compile --disable-amd3dnow"
else
  config="$config --disable-amd3dnow"
fi

#config="$config --enable-libvorbis --enable-muxer=ogg --enable-encoder=libvorbis"
config="$config --disable-libvorbis"
config="$config --enable-gpl --enable-postproc --enable-static --enable-pthreads"
config="$config --enable-muxer=spdif --enable-muxer=adts --enable-encoder=ac3 --enable-encoder=aac"
config="$config --enable-protocol=http --enable-runtime-cpudetect --enable-gnutls"
config="$config --prefix=$ROOT/plex/Dependencies/xbmc-depends/ffmpeg-$outputdir"
config="$config --cc=clang"

extra_cflags="$extra_cflags -I$outputpath/include"

#GNUTLS only uses pkg-config, we need to export this for it to work
export PKG_CONFIG_PATH="$outputpath/lib/pkgconfig:$PKG_CONFIG_PATH"

#echo $config
echo ./configure $config --as="$AS" --extra-cflags="$extra_cflags" --extra-ldflags="-arch $arch -L$outputpath/lib"
./configure $config --as="$AS" --extra-cflags="$extra_cflags" --extra-ldflags="$LDFLAGS -arch $arch -L$outputpath/lib" || exit 1

sed -ie "s#YASM=yasm#YASM=${outputpath}/bin/yasm#" config.mak
sed -ie "s#YASMDEP=yasm#YASMDEP=${outputpath}/bin/yasm#" config.mak
sed -ie "s# -D_ISOC99_SOURCE -D_POSIX_C_SOURCE=200112 # #" config.mak

make || exit 1
ar d libavcodec/libavcodec.a inverse.o
make install || exit 1

DEPDIR=$ROOT/plex/Dependencies

cd $DEPDIR/xbmc-depends
rm -f $DEPDIR/output
mkdir -p $DEPDIR/built-depends

libs=$(find $outputdir -name *.dylib)

rm -rf symbols-$outputdir
mkdir symbols-$outputdir
for l in $libs; do
  dsymutil -o symbols-$outputdir/$(basename $l).dSYM $l
  $outputdir/bin/dump_syms symbols/$outputdir/$(basename $l).dSYM | bzip2 > symbols-$outputdir/$(basename $l).sym.bz2
  rm -rf symbols-$outputdir/$(basename $l).dSYM
  strip -S $l 
done

for l in $libs; do
  codesign --force --sign "Developer ID Application: Plex Inc." $l
done

# wait until mds has calmed down
sleep 20

echo "Packing xbmc-depends"
echo gtar --xz -cf $DEPDIR/built-depends/$outputdir-xbmc-$DEPEND_HASH.tar.xz $outputdir
gtar --xz -cf $DEPDIR/built-depends/$outputdir-xbmc-$DEPEND_HASH.tar.xz $outputdir

echo "Packing symbols"
echo gtar -cf $DEPDIR/built-depends/$outputdir-xbmc-symbols-$DEPEND_HASH.tar symbols-$outputdir
gtar -cf $DEPDIR/built-depends/$outputdir-xbmc-symbols-$DEPEND_HASH.tar symbols-$outputdir

echo "Packing ffmpeg"
echo gtar --xz -cf $DEPDIR/built-depends/$outputdir-ffmpeg-$FFMPEG_HASH.tar.xz ffmpeg-$outputdir
gtar --xz -cf $DEPDIR/built-depends/$outputdir-ffmpeg-$FFMPEG_HASH.tar.xz ffmpeg-$outputdir
