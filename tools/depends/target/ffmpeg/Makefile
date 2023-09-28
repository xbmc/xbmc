-include ../../Makefile.include
include FFMPEG-VERSION
DEPS = FFMPEG-VERSION Makefile ../../download-files.include

BUILD_TYPE = Release
ifeq ($(DEBUG_BUILD),yes)
  BUILD_TYPE = Debug
endif

CMAKE_ARGS=-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
           -DCMAKE_MODULE_PATH="$(CMAKE_SOURCE_DIR)/cmake/modules;$(CMAKE_SOURCE_DIR)/cmake/modules/buildtools" \
           -DPKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig \
           -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
           -DFFMPEG_VER=$(VERSION) \
           -DENABLE_DAV1D=ON \
           -DEXTRA_FLAGS="$(FFMPEG_CONFIGURE_OPTIONS)"

ifeq ($(USE_CCACHE), yes)
  CMAKE_ARGS+= -DENABLE_CCACHE=$(USE_CCACHE) \
               -DCCACHE_PROGRAM=$(CCACHE)
endif

ifeq ($(CROSS_COMPILING), yes)
  DEPS += ../../Makefile.include
  CMAKE_ARGS+= -DPKG_CONFIG_EXECUTABLE=$(NATIVEPREFIX)/bin/pkg-config \
               -DCROSSCOMPILING=$(CROSS_COMPILING)
               -DCMAKE_AR=$(AR) \
               -DCMAKE_RANLIB=$(RANLIB) \
               -DCMAKE_STRIP=$(STRIP)
else
  TARBALLS_LOCATION := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
  RETRIEVE_TOOL = curl
  RETRIEVE_TOOL_FLAGS = -LsS --create-dirs --retry 10 --retry-connrefused -o
  ARCHIVE_TOOL_FLAGS = --strip-components=1 -xf
  BASE_URL := http://mirrors.kodi.tv/build-deps/sources
endif

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); cp ../CMakeLists.txt ./
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_ARGS) ..

.build-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	touch $@

.installed-$(PLATFORM): .build-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
