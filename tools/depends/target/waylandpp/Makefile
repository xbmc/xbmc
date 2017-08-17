-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=waylandpp
VERSION=0.1.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/src/libwayland-client++.so
CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

ifeq ($(PLATFORM),)
	# Building stand-alone
	CXX_FOR_BUILD := $(CXX)
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	export PKG_CONFIG=$(NATIVEPREFIX)/bin/pkg-config
	export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig
endif

SCONS=scons
export CROSSCXX:=$(CXX)
export CXX=$(CXX_FOR_BUILD)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	$(SCONS) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	PREFIX=$(PREFIX) $(SCONS) -C $(PLATFORM) install
	touch $@

clean:
	$(SCONS) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
