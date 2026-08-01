include ../../Makefile.include LIBDVDREAD-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBDVDREAD-VERSION Makefile ../../download-files.include \
                                  01-all-disableopendir.patch \
                                  02-all-disablesymlink.patch \
                                  03-win-add_defines.patch \
                                  04-win-uwp_compat.patch \
                                  05-win-remove_stat.patch \
                                  06-win-remove_config_h.patch \
                                  07-emscripten-bswap.patch \
                                  08-win-ssize_t.patch

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson setup \
  --prefix=$(PREFIX) \
  --libdir=lib \
  --buildtype=$(MESON_BUILD_TYPE) \
  -Denable_docs=false \
  -Dlibdvdcss=enabled \
  . build

ifeq ($(CROSS_COMPILING), yes)
  CONFIGURE += --cross-file=$(PREFIX)/share/cross-file.meson
  export CC=$(CC_FOR_BUILD)
  export CXX=$(CXX_FOR_BUILD)
  export CFLAGS=$(CFLAGS_FOR_BUILD)
  export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
  export CC CXX CFLAGS CXXFLAGS
endif
export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

LIBDYLIB=$(PLATFORM)/build/src/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-disableopendir.patch
	cd $(PLATFORM); patch -p1 -i ../02-all-disablesymlink.patch
ifeq ($(TARGET_PLATFORM),windows)
	cd $(PLATFORM); patch -p1 -i ../03-win-add_defines.patch
	cd $(PLATFORM); patch -p1 -i ../04-win-uwp_compat.patch
	cd $(PLATFORM); patch -p1 -i ../05-win-remove_stat.patch
	cd $(PLATFORM); patch -p1 -i ../06-win-remove_config_h.patch
	cd $(PLATFORM); patch -p1 -i ../08-win-ssize_t.patch
endif
ifeq ($(OS),wasm)
	cd $(PLATFORM); patch -p1 -i ../07-emscripten-bswap.patch
endif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
