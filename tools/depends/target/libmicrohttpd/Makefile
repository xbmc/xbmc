include ../../Makefile.include LIBMICROHTTPD-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBMICROHTTPD-VERSION ../../download-files.include \
                                  01-win-cmake.patch

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --disable-doc \
                      --disable-examples \
                      --disable-curl \
                      --enable-https

ifeq ($(DEBUG_BUILD), yes)
  CONFIGURE+= --enable-asserts
endif

LIBDYLIB=$(PLATFORM)/src/microhttpd/.libs/$(LIBNAME).a

export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),windows)
	cd $(PLATFORM); patch -p1 -i ../01-win-cmake.patch
endif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
