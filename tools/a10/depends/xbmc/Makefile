include ../depends.mk

CONFIGURE=./configure \
	--prefix=$(PREFIX) \
	--build=$(BUILD) \
	--host=$(HOST) \
	--disable-x11 \
	--disable-sdl \
	--disable-xrandr \
	--disable-joystick \
	--disable-gl \
	--disable-vdpau \
	--disable-vaapi \
	--disable-openmax \
	--enable-neon \
	--enable-gles \

SRC=../../../../

all:
	(cd $(SRC); ./bootstrap)
	(cd $(SRC); $(CONFIGURE))
	(cd $(SRC); make)
