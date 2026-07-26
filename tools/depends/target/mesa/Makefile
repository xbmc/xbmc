include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=mesa
VERSION=26.1.5
ARCHIVE=$(LIBNAME)-$(VERSION).tar.xz
BASE_URL=https://archive.mesa3d.org
SHA512=8b1714e0fe63d3077c0d761245680724537a94db65e17dedb7124c7c861f070a13493a060695e81b2ca9b6889007b52c7b5788dace8fe5d8e2e1ed67efdbbf07
include ../../download-files.include

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

ifeq ($(CPU), x86_64)
  MESA_GALLIUM_DRIVERS=iris
else ifeq ($(CPU), $(filter $(CPU), arm aarch64))
  MESA_GALLIUM_DRIVERS=vc4,softpipe,panfrost
endif

ifeq ($(RENDER_SYSTEM),gles)
  MESA_EXTRA=-Dplatforms=wayland
endif

CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--libdir=lib \
	--buildtype=$(MESON_BUILD_TYPE) \
	-Dgallium-drivers="$(MESA_GALLIUM_DRIVERS)" \
	-Dgallium-extra-hud=false \
	-Dvulkan-drivers= \
	-Dshader-cache=true \
	-Dshared-glapi=enabled \
	-Dopengl=true \
	-Dgbm=enabled \
	-Degl=enabled \
	-Dvalgrind=false \
	-Dlibunwind=false \
	-Dlmsensors=false \
	-Dbuild-tests=false \
	-Dselinux=false \
	-Dglx=disabled \
	-Dglvnd=false \
	-Dllvm=false \
	-Dgallium-va=disabled \
	-Dgles1=disabled \
	-Dgles2=enabled \
	$(MESA_EXTRA)

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

LIBDYLIB=$(PLATFORM)/build/src/egl/libEGL.so

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
