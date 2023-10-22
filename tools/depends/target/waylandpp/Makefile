-include ../../Makefile.include
include WAYLANDPP-VERSION ../../download-files.include
DEPS =Makefile ../../download-files.include WAYLANDPP-VERSION 001-fix-gcc13-build.patch

LIBDYLIB=$(PLATFORM)/build/libwayland-client++.so

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
	CMAKE := cmake
	CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_INSTALL_LIBDIR=lib -DBUILD_SCANNER=ON $(CMAKE_OPTIONS)
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	CMAKE_OPTIONS := -DBUILD_SCANNER=OFF -DWAYLAND_SCANNERPP=$(NATIVEPREFIX)/bin/wayland-scanner++ $(CMAKE_OPTIONS)
endif

CMAKE_OPTIONS := -DBUILD_DOCUMENTATION=OFF -DBUILD_LIBRARIES=ON -DBUILD_SHARED_LIBS=OFF -DBUILD_EXAMPLES=OFF $(CMAKE_OPTIONS)
BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-fix-gcc13-build.patch
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
ifneq ($(PLATFORM), native)
	# We want to use the native wayland-scanner++
	ln -sf $(NATIVEPREFIX)/lib/pkgconfig/wayland-scanner++.pc $(PREFIX)/lib/pkgconfig/wayland-scanner++.pc
endif
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
