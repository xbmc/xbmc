-include ../../Makefile.include
include RAPIDJSON-VERSION
DEPS = Makefile RAPIDJSON-VERSION 0001-remove_custom_cxx_flags.patch ../../download-files.include

CMAKE_OPTIONS=-DRAPIDJSON_HAS_STDSTRING=ON -DRAPIDJSON_BUILD_DOC=OFF -DRAPIDJSON_BUILD_EXAMPLES=OFF -DRAPIDJSON_BUILD_TESTS=OFF -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=OFF

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
else
  CXXFLAGS += -std=c++11
  ROOT_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

  ifeq ($(PLATFORM),)
    PLATFORM = native
    TARBALLS_LOCATION = $(ROOT_DIR)
    BASE_URL := http://mirrors.kodi.tv/build-deps/sources
    RETRIEVE_TOOL := curl
    RETRIEVE_TOOL_FLAGS := -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar
    ARCHIVE_TOOL_FLAGS := --strip-components=1 -xf
    HASH_TOOL := sha512sum
    HASH_TOOL_FLAGS = -c --status
    CMAKE := cmake
    CMAKE_OPTIONS := -DCMAKE_INSTALL_PREFIX=$(PREFIX) $(CMAKE_OPTIONS)
  endif
endif

include ../../download-files.include

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. $(MAKE) PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../0001-remove_custom_cxx_flags.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	$(MAKE) -C $(PLATFORM)/build install
	rm -rf $(PREFIX)/share/doc/RapidJSON
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
