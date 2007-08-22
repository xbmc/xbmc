./configure --extra-cflags="-D_XBOX" --extra-ldflags="" --extra-libs="" \
	--cpu=pentium3 \
	--arch=i686 \
	--enable-shared \
	--enable-pp \
	--enable-memalign-hack \
	--enable-gpl \
	--enable-mmx \
      --disable-static \
	--disable-altivec \
	--disable-audio-oss \
	--disable-audio-beos \
	--disable-v4l \
	--disable-bktr \
	--disable-dv1394 \
	--disable-network \
	--disable-protocols \
	--disable-zlib \
	--disable-vhook \
	--disable-ffserver \
	--disable-ffmpeg \
	--disable-ffplay \
	--disable-muxers \
	--disable-encoders \
	--disable-ipv6 \
	$@


	
