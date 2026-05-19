include ../../Makefile.include TINYXML2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile TINYXML2-VERSION ../../download-files.include \
                                       001-debug-pdb.patch

CMAKE_OPTIONS=-Dtinyxml2_BUILD_TESTING=OFF \
              -Dtinyxml2_INSTALL_PKGCONFIG=OFF

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); sed -ie 's|\r$$||' CMakeLists.txt
	cd $(PLATFORM); patch -p1 -i ../001-debug-pdb.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) -DCMAKE_BUILD_TYPE=Debug ..
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
