include ../../Makefile.include HIGHWAY-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile HIGHWAY-VERSION ../../download-files.include

CMAKE_OPTIONS= -DHWY_ENABLE_CONTRIB=OFF \
               -DHWY_ENABLE_EXAMPLES=OFF \
               -DHWY_ENABLE_TESTS=OFF \
               -DHWY_TEST_STANDALONE=OFF

LIBDYLIB=$(PLATFORM)/build-lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build-lib
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build-lib; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build-lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build-lib install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build-lib clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
