include ../../Makefile.include WAYLAND-SCANNER-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile WAYLAND-SCANNER-VERSION ../../download-files.include

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson setup \
	--prefix $(PREFIX) \
	--libdir $(PREFIX)/lib \
	--buildtype=release \
	-Dlibraries=false \
	-Dtests=false \
	-Ddocumentation=false \
	-Ddtd_validation=false

export CC=$(CC_BINARY_FOR_BUILD)
export CXX=$(CXX_BINARY_FOR_BUILD)
export CFLAGS=$(NATIVE_CFLAGS)
export CXXFLAGS=$(NATIVE_CXXFLAGS)
export LDFLAGS=$(NATIVE_LDFLAGS)

export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
