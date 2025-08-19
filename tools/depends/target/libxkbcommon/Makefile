include ../../Makefile.include LIBXKBCOMMON-VERSION ../../download-files.include
DEPS =../../Makefile.include LIBXKBCOMMON-VERSION Makefile 0001-No-error-on-out-of-bounds-keycode.patch ../../download-files.include

# configuration settings
MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
                --buildtype=$(MESON_BUILD_TYPE) \
                --prefix=$(PREFIX) \
                -Ddefault_library=static \
                -Denable-docs=false \
                -Denable-x11=false \
                -Denable-wayland=false \
                -Denable-xkbregistry=false \
                -Dxkb-config-root=/usr/share/X11/xkb

ifeq ($(CROSS_COMPILING), yes)
CONFIGURE += --cross-file $(PREFIX)/share/cross-file.meson
endif

LIBDYLIB=$(PLATFORM)/build/libxkbcommon.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0001-No-error-on-out-of-bounds-keycode.patch
	cd $(PLATFORM); $(CONFIGURE) . build

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
