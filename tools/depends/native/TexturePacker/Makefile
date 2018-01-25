ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include ../../Makefile.include
DEPS=Makefile

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
  DEPS += ../../Makefile.include
endif

ifeq ($(NATIVE_OS), linux)
  EXTRA_CONFIGURE = --enable-static
endif
ifeq ($(NATIVE_OS), android)
  EXTRA_CONFIGURE = --enable-static
endif

ifeq ($(CMAKE_SOURCE_DIR),)
  CMAKE_SOURCE_DIR = $(ROOT_DIR)/../../../..
endif

SOURCE=$(ROOT_DIR)/src

APP=$(PLATFORM)/TexturePacker
APPBIN=$(PREFIX)/bin/TexturePacker

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); cp -a $(SOURCE)/* .
	cd $(PLATFORM); ./autogen.sh
	cd $(PLATFORM); ./configure --prefix=$(PREFIX) $(EXTRA_CONFIGURE) EXTRA_DEFINES="$(NATIVE_ARCH_DEFINES)"


$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@
	#TEMP workaround for skins: create legacy link. Remove me when skins are fixed
	@mkdir -p $(CMAKE_SOURCE_DIR)/tools/TexturePacker
	@[ -f $(CMAKE_SOURCE_DIR)/tools/TexturePacker/TexturePacker ] && rm $(CMAKE_SOURCE_DIR)/tools/TexturePacker/TexturePacker || :
	@ln -sf $(APPBIN) $(CMAKE_SOURCE_DIR)/tools/TexturePacker/TexturePacker
	@echo "all:" > $(CMAKE_SOURCE_DIR)/tools/TexturePacker/Makefile
	@echo "\t@echo "WARNING: use of tools/TexturePacker/TexturePacker is deprecated, please update your skins Makefile"" >> $(CMAKE_SOURCE_DIR)/tools/TexturePacker/Makefile

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	-rm -rf bin
