
#./autogen.sh noconfig

export CFLAGS="-D_XBOX -DHAVE_DVDCSS_DVDCSS_H=1 -DNDEBUG "
#export CFLAGS="-D_XBOX -D_MSC_VER=1300 -DHAVE_DVDCSS_DVDCSS_H=1 -D_DEBUG -DDEBUG -I$PWD/xbox/include -Wl,-Map=test.MAP"
export LDFLAGS="libDVDCSS/libdvdcss/src/*.o libDVDCSS/libdvdcss/src/.libs/*.o"

./configure --enable-shared --disable-static

