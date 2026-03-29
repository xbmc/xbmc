include ../../Makefile.include ZLIB-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile ZLIB-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DINSTALL_PKGCONFIG_DIR=$(PREFIX)/lib/pkgconfig \
              -DZLIB_BUILD_EXAMPLES=OFF \
              -DZLIB_BUILD_SHARED=OFF \
			  -DZLIB_BUILD_STATIC=ON

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
