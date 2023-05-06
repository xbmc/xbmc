include ../../Makefile.include
include DAV1D-VERSION
DEPS = ../../Makefile.include Makefile DAV1D-VERSION ../../download-files.include

PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

PYTHON = $(NATIVEPREFIX)/bin/python3
MESON = $(NATIVEPREFIX)/bin/meson
NINJA = $(NATIVEPREFIX)/bin/ninja

MESON_BUILD_TYPE=release

# configuration settings
CONFIGURE = $(PYTHON) $(MESON) \
  --prefix $(PREFIX) \
  --libdir $(PREFIX)/lib \
	--buildtype=$(MESON_BUILD_TYPE) \
	--default-library=static \
	-Denable_asm=true \
	-Denable_tools=false \
	-Denable_examples=false \
	-Denable_tests=false

export CC CXX CFLAGS CXXFLAGS
export PKG_CONFIG_LIBDIR=$(NATIVEPLATFORM)/lib/pkgconfig

include ../../download-files.include
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM)/build; $(NINJA) -v
	cd $(PLATFORM)/build; $(NINJA) -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
