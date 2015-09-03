#!/bin/bash

MAKEFLAGS=""
BGPROCESSFILE="$2"

BASE_URL=$(grep "BASE_URL=" ../../tools/depends/target/ffmpeg/FFMPEG-VERSION | sed 's/BASE_URL=//g')
VERSION=$(grep "VERSION=" ../../tools/depends/target/ffmpeg/FFMPEG-VERSION | sed 's/VERSION=//g')
LIBNAME=ffmpeg
ARCHIVE=$LIBNAME-$VERSION.tar.gz

CUR_DIR=`pwd`
DEPS_DIR=$CUR_DIR/../BuildDependencies
LIB_DIR=$CUR_DIR/../../lib/win32
WGET=$DEPS_DIR/bin/wget
UNZIP=$CUR_DIR/tools/7z/7za

cd $LIB_DIR

if [ "$1" == "clean" ]
then
  echo removing $LIBNAME
  if [ -d $LIBNAME ]
  then
    rm -r $LIBNAME
  fi
fi

if [ ! -d $LIBNAME ]; then
  if [ ! -f $VERSION.tar.gz ]; then
    $WGET --no-check-certificate $BASE_URL/$VERSION.tar.gz -O $VERSION.tar.gz
  fi
  $UNZIP x -y $VERSION.tar.gz
  $UNZIP x -y $VERSION.tar
  mv $LIBNAME-$VERSION $LIBNAME
  cd $LIBNAME
else
  cd $LIBNAME
  if [ -d .libs ]; then
    rm -r .libs
  fi
fi

if [ ! -d .libs ]; then
  mkdir .libs
fi

if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  if [ $NUMBER_OF_PROCESSORS > 4 ]; then
    MAKEFLAGS=-j6
  else
    MAKEFLAGS=-j`expr $NUMBER_OF_PROCESSORS + $NUMBER_OF_PROCESSORS / 2`
  fi
fi

# add --enable-debug (remove --disable-debug ofc) to get ffmpeg log messages in xbmc.log
# the resulting debug dll's are twice to fourth time the size of the release binaries

OPTIONS="
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-zlib \
--disable-static \
--disable-debug \
--disable-ffplay \
--disable-ffserver \
--disable-ffmpeg \
--disable-ffprobe \
--disable-devices \
--disable-doc \
--disable-crystalhd \
--enable-muxer=spdif \
--enable-muxer=adts \
--enable-muxer=asf \
--enable-muxer=ipod \
--enable-muxer=ogg \
--enable-encoder=ac3 \
--enable-encoder=aac \
--enable-encoder=wmav2 \
--enable-encoder=libvorbis \
--enable-protocol=http \
--enable-runtime-cpudetect \
--enable-dxva2 \
--cpu=i686 \
--enable-gnutls"

echo configuring $LIBNAME
./configure --extra-cflags="-fno-common -I/xbmc/lib/win32/ffmpeg_dxva2 -DNDEBUG" --extra-ldflags="-L/xbmc/system/players/dvdplayer" ${OPTIONS} &&

make $MAKEFLAGS &&
cp lib*/*.dll .libs/ &&
cp lib*/*.lib .libs/ &&
cp .libs/avcodec-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avformat-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avutil-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/avfilter-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/postproc-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/swresample-*.dll /xbmc/system/players/dvdplayer/ &&
cp .libs/swscale-*.dll /xbmc/system/players/dvdplayer/

#remove the bgprocessfile for signaling the process end
if [ -f $BGPROCESSFILE ]; then
  rm $BGPROCESSFILE
fi