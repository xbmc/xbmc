include ../../Makefile.include ZSTD-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile ZSTD-VERSION ../../download-files.include

CMAKE_OPTIONS = -DCMAKE_BUILD_TYPE=Release \
                  -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
                  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                  -DZSTD_BUILD_STATIC=ON \
                  -DZSTD_BUILD_SHARED=OFF \
                  -DZSTD_LEGACY_SUPPORT=OFF \
                  -DZSTD_BUILD_PROGRAMS=OFF \
                  -DZSTD_BUILD_TESTS=OFF


LIBDYLIB=$(PLATFORM)/builddir/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/builddir
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/builddir; $(CMAKE) ../build/cmake ${CMAKE_OPTIONS} ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/builddir

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/builddir install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
