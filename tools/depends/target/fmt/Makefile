-include ../../Makefile.include
include FMT-VERSION
DEPS = Makefile FMT-VERSION ../../download-files.include \
                            001-windows-pdb-symbol-gen.patch

CMAKE_OPTIONS=-DCMAKE_CXX_STANDARD=17 -DCMAKE_CXX_EXTENSIONS:BOOL=OFF -DFMT_DOC=OFF -DFMT_INSTALL=ON -DFMT_TEST=OFF

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -LsS --create-dirs --retry 10 --retry-connrefused -o
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
    CMAKE := cmake
    CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_INSTALL_LIBDIR=lib $(CMAKE_OPTIONS)
    HASH_TOOL := sha512sum
  endif
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

include ../../download-files.include

.PHONY: .installed-native

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../001-windows-pdb-symbol-gen.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

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
