include ../../Makefile.include LIBLZO2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBLZO2-VERSION ../../download-files.include

# configuration options
CMAKE_OPTIONS=-DCMAKE_BUILD_TYPE=Release \
              -DENABLE_STATIC=ON \
              -DENABLE_SHARED=OFF \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX)

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
