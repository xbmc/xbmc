include ../../Makefile.include WEBOS-USERLAND-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile WEBOS-USERLAND-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=ON \
              -DENABLE_TESTS=OFF \
              -DENABLE_DOCUMENTATION=OFF \
              -DENABLE_UTILS=OFF \
              -DENABLE_EXAMPLES=OFF

LIBDYLIB=$(PLATFORM)/build/libEGL.so

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); mkdir -p build
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
