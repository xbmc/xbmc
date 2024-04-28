include ../../Makefile.include BROTLI-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile BROTLI-VERSION ../../download-files.include \
                01-all-disable-exe.patch

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=OFF \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DCMAKE_BUILD_TYPE=Release \
              -DBROTLI_DISABLE_TESTS=ON \
              -DBROTLI_DISABLE_EXE=ON

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-disable-exe.patch
	cd $(PLATFORM)/build; $(CMAKE) ${CMAKE_OPTIONS} ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

