include ../../Makefile.include LIBNFS-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile LIBNFS-VERSION ../../download-files.include \
                             001-fix-cmake-build.patch

# configuration settings
CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=OFF \
              -DENABLE_TESTS=OFF \
              -DENABLE_DOCUMENTATION=OFF \
              -DENABLE_UTILS=OFF \
              -DENABLE_EXAMPLES=OFF

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../001-fix-cmake-build.patch
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
