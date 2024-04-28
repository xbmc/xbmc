include ../../Makefile.include TINYXML2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile TINYXML2-VERSION ../../download-files.include \
                                       001-debug-pdb.patch \
                                       002-multiconfig-gen-pkgconfig.patch

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); sed -ie 's|\r$$||' CMakeLists.txt
	cd $(PLATFORM); patch -p1 -i ../001-debug-pdb.patch
	cd $(PLATFORM); patch -p1 -i ../002-multiconfig-gen-pkgconfig.patch
	cd $(PLATFORM)/build; $(CMAKE) -Dtinyxml2_BUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Debug ..

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
