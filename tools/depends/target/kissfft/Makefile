include ../../Makefile.include KISSFFT-VERSION ../../download-files.include
DEPS = ../../Makefile.include KISSFFT-VERSION Makefile ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DKISSFFT_STATIC=ON \
              -DKISSFFT_TOOLS=OFF \
              -DKISSFFT_PKGCONFIG=OFF \
              -DCMAKE_INSTALL_LIBDIR=$(PREFIX)/lib \
              -DCMAKE_INSTALL_INCLUDEDIR=$(PREFIX)/include \
              -DCMAKE_BUILD_TYPE=Release \
              -DKISSFFT_TEST=OFF

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
