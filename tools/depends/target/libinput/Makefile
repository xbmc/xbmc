include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libinput
VERSION=1.10.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=61f19ea777b523f0e76869b8e7dbbff5e51132d3c856c6edb9915a695f7903256228e71be2567642866923a800f1283bdd5aff97266ac26c2e42dfce0bffa50b
include ../../download-files.include

# configuration settings
CONFIGURE= $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
  --prefix=$(PREFIX) \
  --libdir=lib \
  -Ddebug-gui=false \
  -Ddocumentation=false \
  -Dlibwacom=false \
  -Dtests=false

ifeq ($(CROSS_COMPILING), yes)
CONFIGURE += --cross-file $(PREFIX)/share/cross-file.meson
export CC=$(CC_FOR_BUILD)
export CXX=$(CXX_FOR_BUILD)
export CFLAGS=$(CFLAGS_FOR_BUILD)
export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
export CC CXX CFLAGS CXXFLAGS
endif
export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

LIBDYLIB=$(PLATFORM)/build/libinput.so

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build;  $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build;  $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
