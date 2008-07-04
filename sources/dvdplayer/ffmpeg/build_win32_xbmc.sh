./configure --extra-cflags="-D_XBOX" \
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-w32threads \
--enable-postproc \
--enable-swscale \
--enable-protocol=http \
--disable-static \
--disable-altivec \
--disable-vhook \
--disable-ffserver \
--disable-ffmpeg \
--disable-ffplay \
--disable-muxers \
--disable-encoders \
--disable-ipv6 \
--disable-debug && 
 
make -j3 && 
strip lib*/*.dll
