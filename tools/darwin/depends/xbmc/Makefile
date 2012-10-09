include ../Makefile.include

SOURCE=../../../../

ifeq ($(DARWIN), ios)
include ../Makefile.buildtools
export USE_TEXTUREPACKER_NATIVE_ROOT=$(TOOLCHAIN)
else
export USE_TEXTUREPACKER_NATIVE_ROOT=$(PREFIX)
endif

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --enable-upnp \
  --enable-gtest \
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
