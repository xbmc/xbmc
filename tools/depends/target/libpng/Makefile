include ../../Makefile.include LIBPNG-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBPNG-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DPNG_SHARED=OFF \
              -DPNG_STATIC=ON \
              -DPNG_TESTS=OFF \
              -DPNG_DEBUG=OFF

ifneq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  CMAKE_OPTIONS+= -DPNG_HARDWARE_OPTIMIZATIONS=ON \
                  -DPNG_BUILD_ZLIB=ON \
                  -DZLIB_LIBRARY=$(PREFIX)/lib/libz.a \
                  -DZLIB_INCLUDE_DIR=$(PREFIX)/include
else
  ifeq ($(OS), osx)
    CMAKE_OPTIONS+= -DPNG_HARDWARE_OPTIMIZATIONS=ON
  else
    CMAKE_OPTIONS+= -DPNG_HARDWARE_OPTIMIZATIONS=OFF
  endif
endif

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
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
