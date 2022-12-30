include ../../Makefile.include NGHTTP2-VERSION ../../download-files.include
DEPS = ../../Makefile.include NGHTTP2-VERSION Makefile ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DENABLE_DEBUG=OFF \
              -DENABLE_FAILMALLOC=OFF \
              -DENABLE_LIB_ONLY=ON \
              -DENABLE_STATIC_LIB=ON \
              -DENABLE_SHARED_LIB=OFF

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
