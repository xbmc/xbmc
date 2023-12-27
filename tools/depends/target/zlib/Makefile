include ../../Makefile.include ZLIB-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile ZLIB-VERSION ../../download-files.include \
                                        01-all-disable_tests.patch \
                                        02-all-static_only.patch \
                                        03-android-fix_gz_intmax.patch

# configuration settings
CMAKE_OPTIONS=-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DINSTALL_PKGCONFIG_DIR=$(PREFIX)/lib/pkgconfig

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-disable_tests.patch
	cd $(PLATFORM); patch -p1 -i ../02-all-static_only.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../03-android-fix_gz_intmax.patch
endif
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
