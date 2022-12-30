include ../../Makefile.include LIBZIP-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBZIP-VERSION Makefile ../../download-files.include

CMAKE_OPTIONS=-DBUILD_DOC:BOOL=OFF \
              -DBUILD_EXAMPLES:BOOL=OFF \
              -DBUILD_REGRESS:BOOL=OFF \
              -DBUILD_SHARED_LIBS:BOOL=OFF \
              -DBUILD_TOOLS:BOOL=OFF \
              -DCMAKE_BUILD_TYPE=Debug

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CMAKE) -B build ${CMAKE_OPTIONS}

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
