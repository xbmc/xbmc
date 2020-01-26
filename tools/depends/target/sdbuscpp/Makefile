-include ../../Makefile.include
DEPS=Makefile

# lib name, version
LIBNAME=sdbuscpp
VERSION=0.8.1
SOURCE=$(LIBNAME)-$(VERSION)
#ARCHIVE=$(SOURCE).tar.gz
ARCHIVE=v$(VERSION).tar.gz

# todo: remove
BASE_URL := https://github.com/Kistler-Group/sdbus-cpp/archive/

LIBDYLIB=$(PLATFORM)/build/$(LIBNAME)/libsdbus-c++.a

ifeq ($(PLATFORM),)
	# Building stand-alone
	ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
	PLATFORM = native
	TARBALLS_LOCATION = $(ROOT_DIR)
	BASE_URL := https://github.com/Kistler-Group/sdbus-cpp/archive/
	#BASE_URL := http://mirrors.kodi.tv/build-deps/sources
	RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
	ARCHIVE_TOOL := tar --strip-components=1 -xf
	CMAKE := cmake
	CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) $(CMAKE_OPTIONS)
else
	# Building as part of depends
	DEPS += ../../Makefile.include
	CMAKE_OPTIONS := $(CMAKE_OPTIONS)
endif

CMAKE_OPTIONS += -DBUILD_LIBSYSTEMD=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DBUILD_CODE_GEN=OFF -DBUILD_DOC=OFF
CXXFLAGS += -std=c++17
BUILDDIR = $(PLATFORM)/build

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
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

