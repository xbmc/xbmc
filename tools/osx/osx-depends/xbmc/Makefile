#  A quick and dirty Makefile to configure xbmc
#

include ../config.mk

SOURCE=../../../../
# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
  PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig \
  PYTHON=$(PREFIX)/bin/python

all: configure

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make xcode_depends
