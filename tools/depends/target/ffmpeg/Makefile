include ../../Makefile.include
include FFMPEG-VERSION ../../download-files.include
DEPS = ../../Makefile.include FFMPEG-VERSION Makefile ../../download-files.include

BUILD_TYPE = Release
ifeq ($(DEBUG_BUILD),yes)
  BUILD_TYPE = Debug
endif

CMAKE_ARGS=-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
           -DCMAKE_MODULE_PATH=$(CMAKE_SOURCE_DIR)/cmake/modules \
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
  CMAKE_ARGS+= -DPKG_CONFIG_EXECUTABLE=$(NATIVEPREFIX)/bin/pkg-config \
               -DCROSSCOMPILING=$(CROSS_COMPILING)
endif

ifeq ($(OS), android)
  ifeq ($(findstring arm64, $(CPU)), arm64)
  CMAKE_ARGS+= -DENABLE_NEON=YES
  endif
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -Ls --create-dirs -f -o $(TARBALLS_LOCATION)/$(ARCHIVE) $(BASE_URL)/archive/$(VERSION).tar.gz

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
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
