#!/bin/bash
#
#      Copyright (C) 2005-2013 Team XBMC
#      http://xbmc.org
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XBMC; see the file COPYING.  If not, see
#  <http://www.gnu.org/licenses/>.
#


MYDIR=$(cd $(dirname $0); pwd)
cd $MYDIR
FFMPEG_PREFIX=${MYDIR}/ffmpeg-install

BASE_URL=$(grep "BASE_URL=" FFMPEG-VERSION | sed 's/BASE_URL=//g')
VERSION=$(grep "VERSION=" FFMPEG-VERSION | sed 's/VERSION=//g')
ARCHIVE=ffmpeg-$(echo "${VERSION}" | sed 's/\//-/g').tar.gz

function usage {
  echo "usage $(basename $0) 
       [-p | --prefix]    ... ffmepg install prefix
       [-d | --download]  ... no build, download tarfile only
       [-r | --release]   ... disable debugging symbols
       [-j]               ... make concurrency level
       [--cpu=CPU]        ... minimum required CPU
       [--arch=ARCH]      ... select architecture
       [--disable-optimizations]
  "
}

while :
do
  case $1 in
    -h | --help)
      usage
      exit 0
      ;;
    -p | --prefix)
      FFMPEG_PREFIX=$2
      shift 2
      ;; 
    --prefix=*)
      FFMPEG_PREFIX=${1#*=}
      shift
      ;; 
    -d | --download)
      downloadonly=true 
      shift
      ;;
    -r | --release)
      FLAGS="$FLAGS --disable-debug" 
      shift
      ;;
    --disable-optimizations)
      FLAGS="$FLAGS --disable-optimizations"
      shift
      ;;
    --cpu=*)
      FLAGS="$FLAGS --cpu=${1#*=}"
      shift
      ;;
    --arch=*)
      FLAGS="$FLAGS --arch=${1#*=}"
      shift
      ;;
    --extra-cflags=*)
      FLAGS="$FLAGS --extra-cflags=\"${1#*=}\""
      shift
      ;;
    --extra-cxxflags=*)
      FLAGS="$FLAGS --extra-cxxflags=\"${1#*=}\""
      shift
      ;;
    -j)
      BUILDTHREADS=$2
      shift 2
      ;;
    --)
      shift
      break
      ;;
    -*)
      echo "WARN: Unknown option (ignored): $1" >&2
      shift
      ;;
    *)
      break
      ;;
  esac
done

BUILDTHREADS=${BUILDTHREADS:-$(grep -c "^processor" /proc/cpuinfo)}
[ ${BUILDTHREADS} -eq 0 ] && BUILDTHREADS=1

[ -z ${VERSION} ] && exit 3
if [ -f ${FFMPEG_PREFIX}/lib/pkgconfig/libavcodec.pc ] && [ -f .ffmpeg-installed ]
then
  CURVER=$(cat .ffmpeg-installed)
  [ "$VERSION" == "$CURVER" ] && exit 0
fi

[ -f ${ARCHIVE} ] ||
  curl -Ls --create-dirs -f -o ${ARCHIVE} ${BASE_URL}/${VERSION}.tar.gz ||
  { echo "error fetching ${BASE_URL}/${VERSION}.tar.gz" ; exit 3; }
[ $downloadonly ] && exit 0

[ -d ffmpeg-${VERSION} ] && rm -rf ffmpeg-${VERSION} && rm .ffmpeg-installed >/dev/null 2>&1
if [ -d ${FFMPEG_PREFIX} ]
then
  [ -w ${FFMPEG_PREFIX} ] || SUDO="sudo"
else
  [ -w $(dirname ${FFMPEG_PREFIX}) ] || SUDO="sudo"
fi

mkdir -p "ffmpeg-${VERSION}"
cd "ffmpeg-${VERSION}" || exit 2
tar --strip-components=1 -xf $MYDIR/${ARCHIVE}

CFLAGS="$CFLAGS" CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" \
./configure --prefix=$FFMPEG_PREFIX \
	--extra-version="kodi-${VERSION}" \
	--disable-devices \
	--disable-ffplay \
	--disable-ffmpeg \
	--disable-sdl \
	--disable-ffprobe \
	--disable-ffserver \
	--disable-doc \
	--enable-gpl \
	--enable-runtime-cpudetect \
	--enable-postproc \
	--enable-vaapi \
	--enable-vdpau \
	--enable-bzlib \
	--enable-gnutls \
	--enable-muxer=spdif \
	--enable-muxer=adts \
	--enable-muxer=asf \
	--enable-muxer=ipod \
	--enable-encoder=ac3 \
	--enable-encoder=aac \
	--enable-encoder=wmav2 \
	--enable-protocol=http \
	--enable-encoder=png \
	--enable-encoder=mjpeg \
	--enable-nonfree \
	--enable-pthreads \
	--enable-pic \
	--enable-zlib \
	--disable-mipsdsp \
	--disable-mipsdspr2 \
        ${FLAGS}

make -j ${BUILDTHREADS} 
if [ $? -eq 0 ]
then
  [ ${SUDO} ] && echo "Root privileges are required to install to ${FFMPEG_PREFIX}"
  ${SUDO} make install && echo "$VERSION" > $MYDIR/.ffmpeg-installed
else
  echo "ERROR: Building ffmpeg failed"
  exit 1
fi
