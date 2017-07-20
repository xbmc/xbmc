#!/bin/bash

[[ -f buildhelpers.sh ]] &&
    source buildhelpers.sh

do_load_autoconf() {
  do_loaddeps $1
  do_clean_get $MAKEFLAGS
  do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Configuring"
  do_autoreconf
}

LIBDVDCSS_VERSION_FILE=/xbmc/tools/depends/target/libdvdcss/DVDCSS-VERSION
LIBDVDREAD_VERSION_FILE=/xbmc/tools/depends/target/libdvdread/DVDREAD-VERSION 
LIBDVDNAV_VERSION_FILE=/xbmc/tools/depends/target/libdvdnav/DVDNAV-VERSION 

LIBDVDPREFIX=$PREFIX
PKG_CONFIG_PATH=$LIBDVDPREFIX/lib/pkgconfig
export PKG_CONFIG_PATH

#libdvdcss
do_load_autoconf $LIBDVDCSS_VERSION_FILE
CC="gcc -static-libgcc" \
$LOCALSRCDIR/configure \
   --prefix=$LIBDVDPREFIX \
   CFLAGS="-DNDEBUG" \
   --disable-doc \
   --with-pic \
   --build="$MINGW_CHOST"
do_makelib $MAKEFLAGS &&
strip -S $LIBDVDPREFIX/bin/libdvdcss-2.dll

#libdvdread
do_load_autoconf $LIBDVDREAD_VERSION_FILE
CC="gcc -static-libgcc" \
$LOCALSRCDIR/configure \
   --prefix=$LIBDVDPREFIX \
   --disable-shared \
   --enable-static \
   --with-libdvdcss \
   CFLAGS="-DHAVE_DVDCSS_DVDCSS_H -D_XBMC -DNDEBUG -I$LIBDVDPREFIX/include" \
   --build="$MINGW_CHOST"
do_makelib $MAKEFLAGS

#libdvdnav
do_load_autoconf $LIBDVDNAV_VERSION_FILE
CC="gcc -static-libgcc" \
$LOCALSRCDIR/configure \
   --prefix=$LIBDVDPREFIX \
   --disable-shared \
   --enable-static \
   CFLAGS="-D_XBMC -DNDEBUG -I$LIBDVDPREFIX/include" \
   --build="$MINGW_CHOST"
do_makelib $MAKEFLAGS

cd $LOCALBUILDDIR
gcc \
   -shared \
   -o $LIBDVDPREFIX/bin/libdvdnav.dll \
   -ldl \
   libdvdread-$TRIPLET/src/*.o \
   libdvdnav-$TRIPLET/src/*.o \
   libdvdnav-$TRIPLET/src/vm/*.o \
   $LIBDVDPREFIX/lib/libdvdcss.dll.a \
   -Wl,--enable-auto-image-base \
   -Xlinker --enable-auto-import \
   -static-libgcc

strip -S $LIBDVDPREFIX/bin/libdvdnav.dll &&
do_print_status "libdvd (${TRIPLET})" "$green_color" "Done"

exit $?