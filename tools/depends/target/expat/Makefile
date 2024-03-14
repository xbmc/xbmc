include ../../Makefile.include EXPAT-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile EXPAT-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DEXPAT_BUILD_TOOLS=OFF \
              -DEXPAT_BUILD_EXAMPLES=OFF \
              -DEXPAT_BUILD_TESTS=OFF \
              -DEXPAT_SHARED_LIBS=OFF \
              -DEXPAT_BUILD_DOCS=OFF \
              -DEXPAT_BUILD_PKGCONFIG=ON \
              -DCMAKE_BUILD_TYPE=Release

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
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
