include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=libdrm
VERSION=2.4.115
ARCHIVE=$(LIBNAME)-$(VERSION).tar.xz
SHA512=0c38d3cfd76f627b899f052527c2939d5fc87a417422dceb0761839ba21e69736703a87ba170b5ba7a4aca2506a240760c8c97ca1781a7fb78468225295293bd
include ../../download-files.include

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--libdir=lib \
	--buildtype=$(MESON_BUILD_TYPE) \
	-Ddefault_library=shared \
	-Dnouveau=disabled \
	-Domap=disabled \
	-Dexynos=disabled \
	-Dtegra=disabled \
	-Dintel=disabled \
	-Dradeon=disabled \
	-Damdgpu=disabled \
	-Dvmwgfx=disabled \
	-Dvc4=disabled \
	-Dfreedreno=disabled \
	-Detnaviv=disabled \
	-Dcairo-tests=disabled \
	-Dman-pages=disabled \
	-Dvalgrind=disabled \
	-Dfreedreno-kgsl=false \
	-Dinstall-test-programs=false \
	-Dudev=false

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

LIBDYLIB=$(PLATFORM)/build/$(LIBNAME).so

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
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
