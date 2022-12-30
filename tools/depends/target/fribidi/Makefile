include ../../Makefile.include FRIBIDI-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile FRIBIDI-VERSION ../../download-files.include

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--buildtype=$(MESON_BUILD_TYPE) \
	--prefix=$(PREFIX) \
	--cross-file $(PREFIX)/share/cross-file.meson \
	-Ddocs=false \
	-Dtests=false \
	-Dbin=false \
	-Ddefault_library=static

export CC_FOR_BUILD
export CFLAGS_FOR_BUILD = $(NATIVE_CFLAGS)
export LDFLAGS_FOR_BUILD = $(NATIVE_LDFLAGS)

export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

LIBDYLIB=$(PLATFORM)/build/lib/lib$(LIBNAME).a

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
