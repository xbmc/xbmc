include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

LIBNAME=pipewire
VERSION=0.3.63
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=3c1808a2adb3dfe2fcaf5e3c7a0ae2f7f0194e7fe25f22fa6d4aec1665d9d5ffec5294cae57e1f6e4d8a360ab6550c193902f0e9996b0abfee09ed9b80d720aa
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
	-Ddocs=disabled \
	-Dexamples=disabled \
	-Dman=disabled \
	-Dtests=disabled \
	-Dinstalled_tests=disabled \
	-Dgstreamer=disabled \
	-Dgstreamer-device-provider=disabled \
	-Dsystemd=disabled \
	-Dsystemd-system-service=disabled \
	-Dsystemd-user-service=disabled \
	-Dpipewire-alsa=disabled \
	-Dpipewire-jack=disabled \
	-Dpipewire-v4l2=disabled \
	-Djack-devel=false \
	-Dspa-plugins=enabled \
	-Dalsa=disabled \
	-Daudiomixer=enabled \
	-Daudioconvert=enabled \
	-Dbluez5=disabled \
	-Dcontrol=enabled \
	-Daudiotestsrc=disabled \
	-Dffmpeg=disabled \
	-Djack=disabled \
	-Dsupport=enabled \
	-Devl=disabled \
	-Dtest=disabled \
	-Dv4l2=disabled \
	-Ddbus=disabled \
	-Dlibcamera=disabled \
	-Dvideoconvert=disabled \
	-Dvideotestsrc=disabled \
	-Dvolume=enabled \
	-Dvulkan=disabled \
	-Dpw-cat=disabled \
	-Dudev=disabled \
	-Dudevrulesdir=/usr/lib/udev/rules.d \
	-Dsdl2=disabled \
	-Dsndfile=disabled \
	-Dlibpulse=disabled \
	-Droc=disabled \
	-Davahi=disabled \
	-Decho-cancel-webrtc=disabled \
	-Dlibusb=disabled \
	-Dsession-managers=[] \
	-Draop=disabled \
	-Dlv2=disabled \
	-Dx11=disabled \
	-Dx11-xfixes=disabled \
	-Dlibcanberra=disabled \
	-Dlegacy-rtkit=false \
	-Dflatpak=disabled

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

LIBDYLIB=$(PLATFORM)/build/src/pipewire/libpipewire-0.3.so

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
