#!/bin/bash

[[ -f buildhelpers.sh ]] &&
    source buildhelpers.sh

FFMPEG_CONFIG_FILE=/xbmc/tools/buildsteps/win32/fmpeg_options.txt
FFMPEG_VERSION_FILE=/xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION
FFMPEG_BASE_OPTS="--disable-debug --disable-doc --enable-gpl --enable-gnutls --enable-w32threads"
FFMPEG_DEFAULT_OPTS=""

do_loaddeps $FFMPEG_VERSION_FILE
FFMPEGDESTDIR=/xbmc/lib/win32/$LIBNAME

do_getFFmpegConfig() {
  if [[ -f "$FFMPEG_CONFIG_FILE" ]]; then
    FFMPEG_OPTS_SHARED="$FFMPEG_BASE_OPTS $(cat "$FFMPEG_CONFIG_FILE" | sed -e 's:\\::g' -e 's/#.*//')"
  else
    FFMPEG_OPTS_SHARED="$FFMPEG_BASE_OPTS $FFMPEG_DEFAULT_OPTS"
  fi

  if [[ $BITS = "64bit" ]]; then
    arch=x86_64
    # perhaps it's not optimal
    do_addOption "--cpu=core2"
  else
    arch=x86
    do_addOption "--cpu=i686"
  fi
  export arch

  # add options for static modplug
  if do_checkForOptions "--enable-libmodplug"; then
    do_addOption "--extra-cflags=-DMODPLUG_STATIC"
  fi

  # handle gplv3 libs
  if do_checkForOptions "--enable-libopencore-amrwb --enable-libopencore-amrnb \
    --enable-libvo-aacenc --enable-libvo-amrwbenc"; then
    do_addOption "--enable-version3"
  fi

  do_removeOption "--enable-nonfree"
  do_removeOption "--enable-libfdk-aac"
  do_removeOption "--enable-nvenc"
  do_removeOption "--enable-libfaac"

  # remove libs that don't work with shared
  do_removeOption "--enable-decklink"
  do_removeOption "--enable-libutvideo"
  do_removeOption "--enable-libgme"
}

do_checkForOptions() {
  local isPresent=1
  for option in "$@"; do
    for option2 in $option; do
      if echo "$FFMPEG_OPTS_SHARED" | grep -q -E -e "$option2"; then
        isPresent=0
      fi
    done
  done
  return $isPresent
}

do_addOption() {
  local option=${1%% *}
  local shared=$2
  if ! do_checkForOptions "$option"; then
    FFMPEG_OPTS_SHARED="$FFMPEG_OPTS_SHARED $option"
  fi
}

do_removeOption() {
  local option=${1%% *}
  FFMPEG_OPTS_SHARED=$(echo "$FFMPEG_OPTS_SHARED" | sed "s/ *$option//g")
}

do_getFFmpegConfig

if [[ "$tools" = "msvc" ]]; then
  # this experimental feature for debuging purpose
  do_removeOption "--enable-gnutls"
  do_removeOption "--disable-debug"
  do_addOption "--disable-gnutls"
  do_addOption "--enable-debug"
  do_addOption "--toolchain=msvc"

  PATH="/c/Program Files (x86)/Microsoft Visual Studio 12.0/VC/BIN/":$PATH
  CFLAGS=""
  CXXFLAGS="" 
  LDFLAGS=""
  export PATH CFLAGS CXXFLAGS LDFLAGS

  extra_cflags="-MDd -I$LOCALDESTDIR/include"
  extra_ldflags="-LIBPATH:\"$LOCALDESTDIR/lib\" -LIBPATH:\"/mingw32/lib\" /NODEFAULTLIB:libcmt"
  TARGET_OS=win32
fi

cd $LOCALBUILDDIR

if do_checkForOptions "--enable-gnutls"; then
if do_pkgConfig "gnutls = $GNUTLS_VER"; then
  if [[ ! -f "gnutls-${GNUTLS_VER}.tar.xz" ]]; then
    do_wget "http://mirrors.xbmc.org/build-deps/sources/gnutls-${GNUTLS_VER}.tar.xz"
  fi
  if [ -d "gnutls-${GNUTLS_VER}" ]; then
    rm -r "gnutls-${GNUTLS_VER}"
  fi
  tar -xaf "gnutls-${GNUTLS_VER}.tar.xz"
  cd "gnutls-${GNUTLS_VER}"

  do_print_status "gnutls-${GNUTLS_VER}" "$blue_color" "Configuring"

  rm -rf $LOCALDESTDIR/include/gnutls
  rm -f $LOCALDESTDIR/lib/{libgnutls*,pkgconfig/gnutls.pc}
  rm -f $LOCALDESTDIR/bin-global/{gnutls-*,{psk,cert,srp,ocsp}tool}.exe

  ./configure --prefix=$LOCALDESTDIR --disable-shared --build="$MINGW_CHOST" --disable-cxx \
      --disable-doc --disable-tools --disable-tests --without-p11-kit --disable-rpath \
      --disable-libdane --without-idn --without-tpm --enable-local-libopts --disable-guile
  sed -i 's/-lgnutls *$/-lgnutls -lnettle -lhogweed -lcrypt32 -lws2_32 -lz -lgmp -lintl -liconv/' \
  lib/gnutls.pc
  do_print_status "gnutls-${GNUTLS_VER}" "$blue_color" "Compiling"
  do_makeinstall 
  do_pkgConfig "gnutls = $GNUTLS_VER";
fi
fi

do_clean_get $1
[ -f config.mak ] && make distclean
do_print_status "$LIBNAME-$VERSION (${BITS})" "$blue_color" "Configuring"

[[ -z "$extra_cflags" ]] && extra_cflags=-DPTW32_STATIC_LIB
[[ -z "$extra_ldflags" ]] && extra_ldflags=-static-libgcc

./configure --target-os=mingw32 --prefix=$FFMPEGDESTDIR --arch=$arch \
  --disable-static --enable-shared $FFMPEG_OPTS_SHARED \
  --extra-cflags="$extra_cflags" --extra-ldflags="$extra_ldflags"

do_makelib &&
cp $FFMPEGDESTDIR/bin/*.dll /xbmc/system/ &&

#remove the bgprocessfile for signaling the process end
if [ -f $BGPROCESSFILE ]; then
  rm $BGPROCESSFILE
fi
