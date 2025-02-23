include ../../Makefile.include LIBLZO2-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBLZO2-VERSION ../../download-files.include \
                                001-all-enable_tests.patch \
                                002-all-enable_docs.patch \
                                003-all-install_pkgconfig.patch \
                                004-all-win_set_debug_postfix.patch

# configuration options
CMAKE_OPTIONS=-DCMAKE_BUILD_TYPE=Release \
              -DENABLE_STATIC=ON \
              -DENABLE_SHARED=OFF \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX)

# lzo hasnt been updated since 2017. Force this for cmake 4.0
CMAKE_OPTIONS+= -DCMAKE_POLICY_VERSION_MINIMUM=3.10

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-all-enable_tests.patch
	cd $(PLATFORM); patch -p1 -i ../002-all-enable_docs.patch
	cd $(PLATFORM); patch -p1 -i ../003-all-install_pkgconfig.patch
	cd $(PLATFORM); patch -p1 -i ../004-all-win_set_debug_postfix.patch
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
