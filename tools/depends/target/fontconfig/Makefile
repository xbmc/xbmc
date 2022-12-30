include ../../Makefile.include FONTCONFIG-VERSION ../../download-files.include
DEPS = ../../Makefile.include FONTCONFIG-VERSION Makefile ../../download-files.include

# configuration settings
MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
                --buildtype=$(MESON_BUILD_TYPE) \
                --prefix=$(PREFIX) \
                -Ddoc=disabled \
                -Dtests=disabled \
                -Dtools=disabled \
                -Ddefault_library=static

ifeq ($(CROSS_COMPILING), yes)
  CONFIGURE += -Dcache-build=disabled \
               --cross-file $(PREFIX)/share/cross-file.meson
endif

LIBDYLIB=$(PLATFORM)/build/src/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE) . build

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
