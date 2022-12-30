include ../../Makefile.include LIBXML2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBXML2-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=OFF \
              -DLIBXML2_WITH_DEBUG=OFF \
              -DLIBXML2_WITH_ICONV=OFF \
              -DLIBXML2_WITH_LZMA=OFF \
              -DLIBXML2_WITH_MODULES=OFF \
              -DLIBXML2_WITH_PYTHON=OFF \
              -DLIBXML2_WITH_RUN_DEBUG=OFF \
              -DLIBXML2_WITH_PROGRAMS=OFF \
              -DLIBXML2_WITH_TESTS=OFF

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
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

