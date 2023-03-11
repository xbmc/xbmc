include ../../Makefile.include FREETYPE2-NOHARFBUZZ-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile FREETYPE2-NOHARFBUZZ-VERSION ../../download-files.include \
                                        01-darwinembedded-incorrecttoolchain.patch

# Freetype has a circular dependency with harfbuzz.
# To enable harfbuzz support in freetype, we build this first to bootstrap harfbuzz
# and then build freetype2 again with harfbuzz support

CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=OFF \
              -DFT_DISABLE_ZLIB=ON \
              -DFT_DISABLE_BZIP2=ON \
              -DFT_DISABLE_PNG=ON \
              -DFT_DISABLE_HARFBUZZ=ON \
              -DFT_DISABLE_BROTLI=ON

ifeq ($(OS), darwin_embedded)
  # We need SDKROOT without the /SDKs/*.sdk for IOS_DEVELOPER_ROOT
  DEV_ROOT=$(shell awk -v sdk=$(SDKROOT) 'BEGIN{ split(sdk, a, /\/SDKs/); print a[1]}')

  CMAKE_OPTIONS+= -DIOS_PLATFORM=OS \
                  -DCMAKE_AR=$(AR) \
                  -DCMAKE_OSX_ARCHITECTURES=$(CPU) \
                  -DCMAKE_IOS_DEVELOPER_ROOT=$(DEV_ROOT) \
                  -DCMAKE_IOS_SDK_ROOT=$(SDKROOT)
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../01-darwinembedded-incorrecttoolchain.patch
endif
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
