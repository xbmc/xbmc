#!/bin/sh

START_PATH=`pwd`
if [ $START_PATH != "/xbmc/lib/libdvd" ]
then
cd /xbmc/lib/libdvd
fi

#libdvdcss
cd libdvdcss
echo "***** Cleaning libdvdcss *****"
make distclean
echo "***** Building libdvdcss *****"
sh bootstrap
./configure \
      CFLAGS="-DNDEBUG" \
      --disable-doc \
      --enable-static \
      --with-pic
make
strip -S src/.libs/libdvdcss-2.dll
cd ..
mkdir -p includes/dvdcss
cp libdvdcss/src/dvdcss/dvdcss.h includes/dvdcss
cp libdvdcss/src/.libs/libdvdcss-2.dll /xbmc/system/players/dvdplayer/

#libdvdread
cd libdvdread
echo "***** Cleaning libdvdread *****"
make distclean
echo "***** Building libdvdread *****"
./configure2 \
      --disable-shared \
      --enable-static \
      --extra-cflags="-DHAVE_DVDCSS_DVDCSS_H -D_XBMC -DNDEBUG -D_MSC_VER -I`pwd`/../includes" \
      --disable-debug
mkdir -p ../includes/dvdread
cp ../libdvdread/src/*.h ../includes/dvdread
make
cd ..

#libdvdnav
cd libdvdnav
echo "***** Cleaning libdvdnav *****"
make distclean
echo "***** Building libdvdnav *****"
./configure2 \
      --disable-shared \
      --enable-static \
      --extra-cflags="-D_XBMC -DNDEBUG -I`pwd`/../includes" \
      --with-dvdread-config="`pwd`/../libdvdread/obj/dvdread-config" \
      --disable-debug
make
gcc \
      -shared \
      -o obj/libdvdnav.dll \
      ../libdvdread/obj/*.o obj/*.o ../libdvdcss/src/.libs/libdvdcss.dll.a \
      -ldl \
      -Wl,--enable-auto-image-base \
      -Xlinker --enable-auto-import

strip -S obj/libdvdnav.dll
cd ..
cp libdvdnav/obj/libdvdnav.dll /xbmc/system/players/dvdplayer/
echo "***** Done *****"

cd $START_PATH