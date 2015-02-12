ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include ../../Makefile.include

ifeq ($(NATIVEPREFIX),)
  PREFIX = $(ROOT_DIR)
else
  PREFIX = $(NATIVEPREFIX)
endif

ifeq ($(NATIVEPLATFORM),)
  PLATFORM = native
  EXTRA_CONFIGURE = --enable-static
else
  PLATFORM = $(NATIVEPLATFORM)
endif

ifeq ($(NATIVE_OS), linux)
  EXTRA_CONFIGURE = --enable-static
endif
ifeq ($(NATIVE_OS), android)
  EXTRA_CONFIGURE = --enable-static
endif

ifeq ($(XBMCROOT),)
  XBMCROOT = $(ROOT_DIR)/../../../..
endif

SOURCE=$(ROOT_DIR)/src

APP=$(PLATFORM)/TexturePacker
APPBIN=$(PREFIX)/bin/TexturePacker

all: .installed-$(PLATFORM)

$(PLATFORM): ../libsquish-native/.installed-$(PLATFORM)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); cp -a $(SOURCE)/* .
	cd $(PLATFORM); ./autogen.sh
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) $(EXTRA_CONFIGURE) EXTRA_DEFINES="$(NATIVE_ARCH_DEFINES)"


$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

../libsquish-native/.installed-$(PLATFORM):
	$(MAKE) -C ../libsquish-native

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@
	#TEMP workaround for skins: create legacy link. Remove me when skins are fixed
	@mkdir -p $(XBMCROOT)/tools/TexturePacker
	@[ -f $(XBMCROOT)/tools/TexturePacker/TexturePacker ] && rm $(XBMCROOT)/tools/TexturePacker/TexturePacker || :
	@ln -s $(APPBIN) $(XBMCROOT)/tools/TexturePacker/TexturePacker
	@echo "all:" > $(XBMCROOT)/tools/TexturePacker/Makefile
	@echo "\t@echo "WARNING: use of tools/TexturePacker/TexturePacker is deprecated, please update your skins Makefile"" >> $(XBMCROOT)/tools/TexturePacker/Makefile

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	-rm -rf bin
