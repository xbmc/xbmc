include ../../Makefile.include LIBPNG-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBPNG-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DPNG_SHARED=OFF \
              -DPNG_STATIC=ON \
              -DPNG_TESTS=OFF \
              -DPNG_DEBUG=OFF \
              -DPNG_HARDWARE_OPTIMIZATIONS=ON

# provide ZLIB_ROOT if platform is building zlib
ifneq ($(HAS_ZLIB),1)
  CMAKE_OPTIONS+= -DZLIB_ROOT=$(PREFIX)
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
