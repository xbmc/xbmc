include ../Makefile.include

SOURCE=../../../../

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig \
  PYTHON=$(PREFIX)/bin/python

all: configure

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make xcode_depends
