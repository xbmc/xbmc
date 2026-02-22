include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

BASE_URL=https://gitlab.freedesktop.org/emersion/libdisplay-info/-/archive/0.2.0
LIBNAME=libdisplay-info
VERSION=0.2.0
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=ada2f7ff94a5b566475f1acf18356b4bec220673072ad5fb34f8da87156b2180ee9a4fafc9c148d847e8c707c25336dbd832d8c89e505cf97ad2c7cec1459f0b
include ../../download-files.include

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--libdir=lib \
	--buildtype=$(MESON_BUILD_TYPE)


ifeq ($(CROSS_COMPILING), yes)
CONFIGURE += --cross-file $(PREFIX)/share/cross-file.meson
export CC=$(CC_FOR_BUILD)
export CXX=$(CXX_FOR_BUILD)
export CFLAGS=$(CFLAGS_FOR_BUILD)
export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
export CC CXX CFLAGS CXXFLAGS
endif
export PKG_CONFIG_LIBDIR=$(PREFIX)/share/pkgconfig

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
