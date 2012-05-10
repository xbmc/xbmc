include ../Makefile.include

SOURCE=../../../../

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-rsxs \
  PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig \
  PYTHON=$(PREFIX)/bin/python

ifeq ($(XBMC_DARWIN_RELEASE),Release)
	CONFIGURE+= --disable-debug
endif

all: configure

configure:
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)
	#cd $(SOURCE); make xcode_depends
