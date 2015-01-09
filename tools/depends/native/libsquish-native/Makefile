DEPS= ../../Makefile.include.in Makefile
ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include ../../Makefile.include

ifeq ($(NATIVEPREFIX),)
  PREFIX = $(ROOT_DIR)/squish-install
else
  PREFIX = $(NATIVEPREFIX)
endif

ifeq ($(NATIVEPLATFORM),)
  PLATFORM = native
else
  PLATFORM = $(NATIVEPLATFORM)
endif

APPNAME=squish
SOURCE=$(ROOT_DIR)/src

DYLIB=$(PLATFORM)/libsquish.a

all: .installed-$(PLATFORM)

$(PLATFORM):
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the xbmc install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); cp -a $(SOURCE)/* .
	cd $(PLATFORM); sed "s|@PREFIX@|$(PREFIX)|" config.in > config
	cd $(PLATFORM); mkdir -p $(PREFIX)/include; mkdir -p $(PREFIX)/lib/pkgconfig

$(DYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(DYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
