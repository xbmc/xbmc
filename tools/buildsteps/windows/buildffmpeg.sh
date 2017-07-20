#!/bin/bash

[[ -f buildhelpers.sh ]] &&
    source buildhelpers.sh

FFMPEG_CONFIG_FILE=/xbmc/tools/buildsteps/windows/ffmpeg_options.txt
FFMPEG_VERSION_FILE=/xbmc/tools/depends/target/ffmpeg/FFMPEG-VERSION
FFMPEG_BASE_OPTS="--disable-debug --disable-doc --enable-gpl --enable-w32threads"
FFMPEG_DEFAULT_OPTS=""
FFMPEG_TARGET_OS=mingw32

do_loaddeps $FFMPEG_VERSION_FILE
FFMPEGDESTDIR=$PREFIX

do_getFFmpegConfig() {
  if [[ -f "$FFMPEG_CONFIG_FILE" ]]; then
    FFMPEG_OPTS_SHARED="$FFMPEG_BASE_OPTS $(cat "$FFMPEG_CONFIG_FILE" | sed -e 's:\\::g' -e 's/#.*//')"
  else
    FFMPEG_OPTS_SHARED="$FFMPEG_BASE_OPTS $FFMPEG_DEFAULT_OPTS"
  fi

  if [ "$ARCH" == "x86_x64" ]; then
    FFMPEG_TARGET_OS=mingw64
  elif [ "$ARCH" == "x86" ]; then
    FFMPEG_TARGET_OS=mingw32
    do_addOption "--cpu=i686"
  elif [ "$ARCH" == "arm" ]; then
    FFMPEG_TARGET_OS=mingw32
    do_addOption "--cpu=armv7"
  fi

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

if [ "$TOOLS" = "msvc" ]; then
  do_removeOption "--enable-gnutls"
  do_removeOption "--enable-openssl"
  do_addOption "--disable-gnutls"
  do_addOption "--disable-openssl"
  do_addOption "--toolchain=msvc"

  if [ "$ARCH" == "x86_x64" ]; then
    FFMPEG_TARGET_OS=win64
  elif [ "$ARCH" = "x86" ]; then
    FFMPEG_TARGET_OS=win32
  elif [ "$ARCH" = "arm" ]; then
    FFMPEG_TARGET_OS=win32
  fi

  export CFLAGS=""
  export CXXFLAGS=""
  export LDFLAGS=""

  extra_cflags="-I$LOCALDESTDIR/include -I/depends/$TRIPLET/include"
  extra_ldflags="-LIBPATH:\"$LOCALDESTDIR/lib\" -LIBPATH:\"$MINGW_PREFIX/lib\" -LIBPATH:\"/depends/$TRIPLET/lib\""
  if [ $win10 == "yes" ]; then
    # enable OpenSSL on UWP, because schannel isn't available
    do_removeOption "--disable-openssl"
    do_addOption "--enable-openssl"
    do_addOption "--enable-nonfree"
    do_addOption "--enable-cross-compile"
    extra_cflags=$extra_cflags" -MD -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WIN32_WINNT=0x0A00"
    extra_ldflags=$extra_ldflags" -APPCONTAINER WindowsApp.lib"
  else
    # compile ffmpeg with debug symbols
    do_removeOption "--disable-debug"
    do_addOption "--enable-debug"
    extra_cflags=$extra_cflags" -MDd"
    extra_ldflags=$extra_ldflags" -NODEFAULTLIB:libcmt"
  fi
fi

cd $LOCALBUILDDIR

if do_checkForOptions "--enable-gnutls"; then
if do_pkgConfig "gnutls = $GNUTLS_VER"; then
  if [[ ! -f "/downloads/gnutls-${GNUTLS_VER}.tar.xz" ]]; then
    do_wget "http://mirrors.xbmc.org/build-deps/sources/gnutls-${GNUTLS_VER}.tar.xz"
  fi
  tar -xaf "/downloads/gnutls-${GNUTLS_VER}.tar.xz" -C $LOCALBUILDDIR/src

  gnutls_triplet="gnutls-${GNUTLS_VER}-$TRIPLET"
  if [[ ! -d "${gnutls_triplet}" ]]; then
    mkdir "${gnutls_triplet}"
  fi
  cd "${gnutls_triplet}"

  do_print_status "gnutls-${GNUTLS_VER}" "$blue_color" "Configuring"

  rm -rf $LOCALDESTDIR/include/gnutls
  rm -f $LOCALDESTDIR/lib/{libgnutls*,pkgconfig/gnutls.pc}
  rm -f $LOCALDESTDIR/bin-global/{gnutls-*,{psk,cert,srp,ocsp}tool}.exe

  $LOCALBUILDDIR/src/gnutls-${GNUTLS_VER}/configure --prefix=$LOCALDESTDIR --disable-shared \
      --build="$MINGW_CHOST" --disable-cxx --disable-doc --disable-tools --disable-tests \
      --without-p11-kit --disable-rpath --disable-libdane --without-idn --without-tpm \
      --enable-local-libopts --disable-guile

  sed -i 's/-lgnutls *$/-lgnutls -lnettle -lhogweed -lcrypt32 -lws2_32 -lz -lgmp -lintl -liconv/' \
  lib/gnutls.pc

  do_print_status "gnutls-${GNUTLS_VER}" "$blue_color" "Compiling"
  do_makeinstall
  do_pkgConfig "gnutls = $GNUTLS_VER";
fi
fi

do_clean_get $1
[ -f config.mak ] && make distclean
do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Configuring"

[[ -z "$extra_cflags" ]] && extra_cflags=-DPTW32_STATIC_LIB
[[ -z "$extra_ldflags" ]] && extra_ldflags=-static-libgcc

$LOCALSRCDIR/configure --target-os=$FFMPEG_TARGET_OS --prefix=$FFMPEGDESTDIR --arch=$ARCH \
  --disable-static --enable-shared $FFMPEG_OPTS_SHARED \
  --extra-cflags="$extra_cflags" --extra-ldflags="$extra_ldflags"

do_makelib
exit $?
