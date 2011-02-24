#  A quick and dirty Makefile to configure xbmc
#

include ../config.mk

SOURCE=../../../../
# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
   --enable-external-python \
  PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

all: configure

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make xcode_depends
