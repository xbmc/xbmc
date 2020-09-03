include ../../Makefile.include
DEPS=../../Makefile.include Makefile

LIBNAME=mesa
VERSION=20.1.7
ARCHIVE=$(LIBNAME)-$(VERSION).tar.xz

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

ifeq ($(CPU), x86_64)
  MESA_GALLIUM_DRIVERS=iris
else ifeq ($(CPU), arm)
  MESA_GALLIUM_DRIVERS=kmsro,vc4
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--libdir=lib \
	--buildtype=$(MESON_BUILD_TYPE) \
	-Ddri-drivers= \
	-Dgallium-drivers="$(MESA_GALLIUM_DRIVERS)" \
	-Dgallium-extra-hud=false \
	-Dgallium-xvmc=false \
	-Dgallium-omx=disabled \
	-Dgallium-nine=false \
	-Dgallium-opencl=disabled \
	-Dvulkan-drivers= \
	-Dshader-cache=true \
	-Dshared-glapi=true \
	-Dopengl=true \
	-Dgbm=true \
	-Degl=true \
	-Dvalgrind=false \
	-Dlibunwind=false \
	-Dlmsensors=false \
	-Dbuild-tests=false \
	-Dselinux=false \
	-Dosmesa=none \
	-Dplatforms="drm" \
	-Ddri3=false \
	-Dglx=disabled \
	-Dglvnd=false \
	-Dllvm=false \
	-Dgallium-vdpau=false \
	-Dgallium-va=false \
	-Dgallium-xa=false \
	-Dgles1=false \
	-Dgles2=true

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

download: $(TARBALLS_LOCATION)/$(ARCHIVE)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
