-include ../../Makefile.include
include CROSSGUID-VERSION
DEPS = Makefile CROSSGUID-VERSION ../../download-files.include \
                                  001-fix-unused-function.patch \
                                  002-disable-Wall-error.patch

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
    RETRIEVE_TOOL := curl -Ls --create-dirs -f -O
    ARCHIVE_TOOL := tar --strip-components=1 -xf
    HASH_TOOL = sha512sum
    HASH_TOOL_FLAGS = -c --status
  endif
endif

CMAKE_OPTIONS=-DCROSSGUID_TESTS=OFF \
              -DCMAKE_BUILD_TYPE=Release \
              -DDISABLE_WALL=ON

LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

include ../../download-files.include

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-fix-unused-function.patch
	cd $(PLATFORM); patch -p1 -i ../002-disable-Wall-error.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
