#!/bin/bash

[[ -f buildhelpers.sh ]] &&
    source buildhelpers.sh

LIBDVDPREFIX=/xbmc/lib/libdvd
PKG_CONFIG_PATH=$LIBDVDPREFIX/lib/pkgconfig
export PKG_CONFIG_PATH

do_load_autoconf() {
  do_loaddeps $1
  do_clean_get $MAKEFLAGS
  do_print_status "$LIBNAME-$VERSION (${BITS})" "$blue_color" "Configuring"
  do_autoreconf
}

#libdvdcss
do_load_autoconf /xbmc/tools/depends/target/libdvdcss/DVDCSS-VERSION
CC="gcc -static-libgcc" \
./configure \
      --prefix=$LIBDVDPREFIX \
      CFLAGS="-DNDEBUG" \
      --disable-doc \
      --with-pic \
      --build="$MINGW_CHOST"
do_makelib $MAKEFLAGS

strip -S $LIBDVDPREFIX/bin/libdvdcss-2.dll &&
cp "$LIBDVDPREFIX/bin/libdvdcss-2.dll" /xbmc/system/

#libdvdread
do_load_autoconf /xbmc/tools/depends/target/libdvdread/DVDREAD-VERSION 
CC="gcc -static-libgcc" \
./configure \
    --prefix=$LIBDVDPREFIX \
   --disable-shared \
   --enable-static \
   --with-libdvdcss \
   CFLAGS="-DHAVE_DVDCSS_DVDCSS_H -D_XBMC -DNDEBUG -I$LIBDVDPREFIX/include" \
   --build="$MINGW_CHOST"
do_makelib $MAKEFLAGS

#libdvdnav
do_load_autoconf /xbmc/tools/depends/target/libdvdnav/DVDNAV-VERSION 
CC="gcc -static-libgcc" \
./configure \
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
   libdvdread/src/*.o libdvdnav/src/*.o libdvdnav/src/vm/*.o $LIBDVDPREFIX/lib/libdvdcss.dll.a \
   -Wl,--enable-auto-image-base \
   -Xlinker --enable-auto-import \
   -static-libgcc

strip -S $LIBDVDPREFIX/bin/libdvdnav.dll &&
cp $LIBDVDPREFIX/bin/libdvdnav.dll /xbmc/system/
do_print_status "libdvd (${BITS})" "$green_color" "Done"

#remove the bgprocessfile for signaling the process end
if [ -f $BGPROCESSFILE ]; then
  rm $BGPROCESSFILE
fi