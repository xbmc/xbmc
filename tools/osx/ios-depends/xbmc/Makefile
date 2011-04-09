#  A quick and dirty Makefile to configure xbmc
#

include ../config.mk

SOURCE=../../../../
# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
  --with-arch=arm --with-cpu=cortex-a8 --enable-gles \
  --disable-optical-drive --disable-dvdcss --enable-optimizations \
  PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

all: configure

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make xcode_depends
