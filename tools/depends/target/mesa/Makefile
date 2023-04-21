include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=mesa
VERSION=23.0.1
ARCHIVE=$(LIBNAME)-$(VERSION).tar.xz
SHA512=9bbe0ba0b1a16fe10b35b6aa3a821d96ab9c4ce4ad38056e2c32271e50b48fd5ef6e1a0babadded631f2b136dfb15acf0c41475d73ee28d132c861ce96517e24
include ../../download-files.include

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

ifeq ($(CPU), x86_64)
  MESA_GALLIUM_DRIVERS=iris
else ifeq ($(CPU), $(filter $(CPU), arm aarch64))
  MESA_GALLIUM_DRIVERS=kmsro,vc4,swrast,panfrost
endif

ifeq ($(RENDER_SYSTEM),gles)
  MESA_EXTRA=-Dplatforms=wayland
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson \
	--prefix=$(PREFIX) \
	--libdir=lib \
	--buildtype=$(MESON_BUILD_TYPE) \
	-Ddri-drivers= \
	-Dgallium-drivers="$(MESA_GALLIUM_DRIVERS)" \
	-Dgallium-extra-hud=false \
	-Dgallium-omx=disabled \
	-Dgallium-nine=false \
	-Dgallium-opencl=disabled \
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
	-Dosmesa=false \
	-Ddri3=false \
	-Dglx=disabled \
	-Dglvnd=false \
	-Dllvm=false \
	-Dgallium-vdpau=false \
	-Dgallium-va=false \
	-Dgallium-xa=false \
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
