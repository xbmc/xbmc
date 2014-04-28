#!/bin/sh

MAKECLEAN=0
MAKEFLAGS=""
BGPROCESSFILE=$2

if [ "$1" = "clean" ]
then
MAKECLEAN=1
fi

if [ $NUMBER_OF_PROCESSORS > 1 ]; then
  MAKEFLAGS=-j$NUMBER_OF_PROCESSORS
fi

#libdvdcss
cd libdvdcss
echo "***** Cleaning libdvdcss *****"
if [ $MAKECLEAN = 1 ]
then
make distclean
fi
echo "***** Building libdvdcss *****"
autoreconf -i
./configure \
      CFLAGS="-DNDEBUG" \
      --disable-doc \
      --enable-static \
      --with-pic
make $MAKEFLAGS
strip -S .libs/libdvdcss-2.dll
cd ..
mkdir -p includes/dvdcss
cp libdvdcss/src/dvdcss/dvdcss.h includes/dvdcss
cp libdvdcss/.libs/libdvdcss-2.dll /xbmc/system/players/dvdplayer/

#libdvdread
cd libdvdread
echo "***** Cleaning libdvdread *****"
if [ $MAKECLEAN = 1 ]
then
make distclean
fi
echo "***** Building libdvdread *****"
./configure2 \
      --disable-shared \
      --enable-static \
      --extra-cflags="-DHAVE_DVDCSS_DVDCSS_H -D_XBMC -DNDEBUG -D_MSC_VER -I`pwd`/../includes" \
      --disable-debug
mkdir -p ../includes/dvdread
cp ../libdvdread/src/dvdread/*.h ../includes/dvdread
make $MAKEFLAGS
cd ..

#libdvdnav
cd libdvdnav
echo "***** Cleaning libdvdnav *****"
if [ $MAKECLEAN = 1 ]
then
make distclean
fi
echo "***** Building libdvdnav *****"
./configure2 \
      --disable-shared \
      --enable-static \
      --extra-cflags="-D_XBMC -DNDEBUG -I`pwd`/../includes" \
      --with-dvdread-config="`pwd`/../dvdread-config" \
      --disable-debug
mkdir -p ../includes/dvdnav
cp ../libdvdnav/src/dvdnav/*.h ../includes/dvdnav
make $MAKEFLAGS
gcc \
      -shared \
      -o obj/libdvdnav.dll \
      ../libdvdread/obj/*.o obj/*.o ../libdvdcss/.libs/libdvdcss.dll.a \
      -ldl \
      -Wl,--enable-auto-image-base \
      -Xlinker --enable-auto-import

strip -S obj/libdvdnav.dll
cd ..
cp libdvdnav/obj/libdvdnav.dll /xbmc/system/players/dvdplayer/
echo "***** Done *****"
#remove the bgprocessfile for signaling the process end
rm $BGPROCESSFILE
