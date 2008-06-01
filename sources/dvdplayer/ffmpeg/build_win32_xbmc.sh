./configure --extra-cflags="-D_XBOX -mno-cygwin -mms-bitfields" --extra-ldflags="-Wl,--add-stdcall-alias" --extra-libs="" \
--arch=i686 \
--cpu=pentium3 \
--enable-shared \
--enable-memalign-hack \
--enable-gpl \
--enable-mmx \
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
--disable-debug \
 
&& make -j3 && strip lib*/*.dll