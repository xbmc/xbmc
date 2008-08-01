#!/bin/sh

# If building on OSX 10.4, you will need to update make from 3.80 to 3.81. 
# Use --prefix=/usr for the configure and since XCode builds have a path
# with "/Developer/usr/bin" need to make/install again using 
# --prefix=/Developer/usr/bin
#
# The postfix name "-osx" must match the postfix name defined in the XCode run script
# call "configure".
#
if [ "$XBMC_ROOT" == "" ]; then
   echo you must define XBMC_ROOT to the root source folder
   exit 1
fi

echo "Configuring ffmpeg"
./configure --extra-cflags="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -I/opt/local/include -L/opt/local/lib -D_XBOX -DHAVE_INT32_T" --extra-ldflags="-L/opt/local/lib" --disable-ffmpeg --disable-ffserver --disable-ffplay --disable-encoders --disable-muxers --disable-debug --enable-gpl --enable-swscale --enable-postproc --enable-libvorbis --enable-protocol=http 

echo "Building ffmpeg"
make clean
make

echo wrapping libavutil
ar rus avutil-51-osx.a libavutil/*.o 

echo wrapping libavcodec
libtool -static -o avcodec-51-osx.a libavcodec/*.o libavcodec/i386/*.o /opt/local/lib/libvorbis.a /opt/local/lib/libvorbisenc.a

echo wrapping libavformat
ar rus avformat-51-osx.a libavformat/*.o

echo wrapping libswscale
ar rus swscale-51-osx.a libswscale/*.o

echo wrapping libpostproc
ar rus postproc-51-osx.a libpostproc/*.o

echo copying libs
cp -v avcodec-51-osx.a avformat-51-osx.a avutil-51-osx.a swscale-51-osx.a postproc-51-osx.a "$XBMC_ROOT"/system/players/dvdplayer

#cp libavcodec/avcodec.h ../../../ffmpeg/
#cp libavformat/avformat.h ../../../ffmpeg/
#cp libavformat/avio.h ../../../ffmpeg/
#cp libavutil/avutil.h ../../../ffmpeg/
#cp libavutil/common.h ../../../ffmpeg/
#cp libavutil/integer.h ../../../ffmpeg/
#cp libavutil/intfloat_readwrite.h ../../../ffmpeg/
#cp libavutil/log.h ../../../ffmpeg/
#cp libavutil/mathematics.h ../../../ffmpeg/
#cp libavutil/mem.h ../../../ffmpeg/
#cp libpostproc/postprocess.h ../../../ffmpeg/
#cp libavutil/rational.h ../../../ffmpeg/
#cp libswscale/rgb2rgb.h ../../../ffmpeg/
#cp libavformat/rtp.h ../../../ffmpeg/
#cp libavformat/rtsp.h ../../../ffmpeg/
#cp libavformat/rtspcodes.h ../../../ffmpeg/
#cp libswscale/swscale.h ../../../ffmpeg/

