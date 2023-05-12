-include ../../Makefile.include
include CROSSGUID-VERSION
DEPS = Makefile CROSSGUID-VERSION ../../download-files.include \
                                  001-fix-unused-function.patch \
                                  002-disable-Wall-error.patch

CMAKE_OPTIONS=-DCROSSGUID_TESTS=OFF \
              -DCMAKE_BUILD_TYPE=Release \
              -DDISABLE_WALL=ON

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
  ifeq ($(PLATFORM),)
    # crossguid uses string_view and requires c++17
    CXXFLAGS += -std=c++17
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
    CMAKE := cmake
    CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_INSTALL_LIBDIR=lib $(CMAKE_OPTIONS)
    HASH_TOOL = sha512sum
    HASH_TOOL_FLAGS = -c --status
  endif
endif

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

include ../../download-files.include

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-fix-unused-function.patch
	cd $(PLATFORM); patch -p1 -i ../002-disable-Wall-error.patch
	cd $(PLATFORM); patch -p1 -i ../003-add-cstdint-include.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
