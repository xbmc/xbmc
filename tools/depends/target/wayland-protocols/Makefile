-include ../../Makefile.include
DEPS =Makefile ../../download-files.include

# lib name, version
LIBNAME=wayland-protocols
VERSION=1.24
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=4b1122517db56f48a5fafd4bd0cb7f94faef6fdd2d80e6cec17e5a6bafbaf2f5a71b958ed12e6d13965494885c209b2fb6dd8331487b39c6f251e71f1e770a15

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl
	RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
	ARCHIVE_TOOL := tar
	ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
	AUTORECONF := autoreconf
else
	# Building as part of depends
	DEPS += ../../Makefile.include
endif

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(PYTHON) $(MESON) $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--buildtype=$(MESON_BUILD_TYPE) \
	-Dtests=false

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	mkdir -p $(PREFIX)/lib/pkgconfig
	ln -sf $(PREFIX)/share/pkgconfig/wayland-protocols.pc $(PREFIX)/lib/pkgconfig/wayland-protocols.pc
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
