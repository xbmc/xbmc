-include ../../Makefile.include
DEPS =Makefile ../../download-files.include

# lib name, version
LIBNAME=waylandpp
VERSION=0.2.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=bf1b8a9e69b87547fc65989b9eaff88a442d8b2f01f5446cef960000b093390b1e557536837fbf38bb6d9a4f93e3985ea34c3253f94925b0f571b4606c980832
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/libwayland-client++.so

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
	CMAKE := cmake
	CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DBUILD_SCANNER=ON $(CMAKE_OPTIONS)
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	CMAKE_OPTIONS := -DBUILD_SCANNER=OFF -DWAYLAND_SCANNERPP=$(NATIVEPREFIX)/bin/wayland-scanner++ $(CMAKE_OPTIONS)
endif

CMAKE_OPTIONS := -DBUILD_DOCUMENTATION=OFF -DBUILD_LIBRARIES=ON -DBUILD_EXAMPLES=OFF $(CMAKE_OPTIONS)
BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install

	# We want to use the native wayland-scanner++
	ln -sf $(NATIVEPREFIX)/lib/pkgconfig/wayland-scanner++.pc $(PREFIX)/lib/pkgconfig/wayland-scanner++.pc
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
