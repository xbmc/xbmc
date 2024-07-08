include ../../Makefile.include EXIV2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile EXIV2-VERSION ../../download-files.include \
            0001-WIN-lib-postfix.patch

# configuration settings
CMAKE_OPTIONS=-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DBUILD_SHARED_LIBS=OFF \
              -DEXIV2_ENABLE_WEBREADY=OFF \
              -DEXIV2_ENABLE_XMP=OFF \
              -DEXIV2_ENABLE_CURL=OFF \
              -DEXIV2_ENABLE_NLS=OFF \
              -DEXIV2_BUILD_SAMPLES=OFF \
              -DEXIV2_BUILD_UNIT_TESTS=OFF \
              -DEXIV2_ENABLE_VIDEO=OFF \
              -DEXIV2_ENABLE_BMFF=OFF \
              -DEXIV2_ENABLE_BROTLI=OFF \
              -DEXIV2_ENABLE_INIH=OFF \
              -DEXIV2_ENABLE_FILESYSTEM_ACCESS=OFF \
              -DEXIV2_BUILD_EXIV2_COMMAND=OFF

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0001-WIN-lib-postfix.patch
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
