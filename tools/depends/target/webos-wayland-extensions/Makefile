-include ../../Makefile.include
include WEBOS-WAYLAND-EXTENSIONS-VERSION
DEPS = WEBOS-WAYLAND-EXTENSIONS-VERSION Makefile \
       01-text-model-v1-abi.patch \
       ../../download-files.include

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
endif

include ../../download-files.include
all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-text-model-v1-abi.patch
	touch $@

.installed-$(PLATFORM): .configured-$(PLATFORM)
	mkdir -p $(PREFIX)/share/wayland-webos $(PREFIX)/lib/pkgconfig
	cp $(PLATFORM)/protocol/*.xml $(PREFIX)/share/wayland-webos
	touch $@

clean:
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
