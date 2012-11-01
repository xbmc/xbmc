include ../../Makefile.include
include ../depends.mk

SOURCE=../../../../

ifeq ($(USE_BUILDROOT),1)
	export PATH:=$(PREFIX)/bin:$(BUILDROOT)/output/host/usr/bin:$(SYSROOT)/usr/bin:$(PATH)
endif

CONFIGURE=./configure --prefix=$(PREFIX) --build=$(BUILD) --host=$(HOST) \
	--enable-gles --disable-sdl --disable-x11 --disable-xrandr --disable-openmax \
	--disable-optical-drive --disable-dvdcss --disable-joystick --disable-debug \
	--disable-crystalhd --disable-vtbdecoder --disable-vaapi --disable-vdpau \
	--disable-pulse --disable-projectm --with-platform=raspberry-pi --enable-optimizations \
	--enable-libcec --enable-player=omxplayer

all: configure

clean:
distclean:
	cd $(SOURCE);

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make -j $(JOBS)
	#cd $(SOURCE); make install
