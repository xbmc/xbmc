include ../../Makefile.include LZO2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LZO2-VERSION ../../download-files.include \
                                001-all-enable_tests.patch \
                                002-all-enable_docs.patch \
                                003-all-cmakeconfig.patch \
                                004-win-pdb.patch

# configuration options
CMAKE_OPTIONS=-DCMAKE_BUILD_TYPE=Release \ 
              -DENABLE_STATIC=ON \
              -DENABLE_SHARED=OFF \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX)

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-all-enable_tests.patch
	cd $(PLATFORM); patch -p1 -i ../002-all-enable_docs.patch
	cd $(PLATFORM); patch -p1 -i ../003-all-cmakeconfig.patch
	cd $(PLATFORM); patch -p1 -i ../004-win-pdb.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
