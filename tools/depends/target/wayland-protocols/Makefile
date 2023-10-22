-include ../../Makefile.include
DEPS =Makefile ../../download-files.include

# lib name, version
LIBNAME=wayland-protocols
VERSION=1.32
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=90bbd52daf342b98823ddeed04e349ae242d2eaf925ab8d603cceb36c980c83b5681bb890961e0d49584cb5c2e60a33abf8821770c6ab87956383630bd5b7966

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
