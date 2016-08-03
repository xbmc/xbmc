include ../Makefile.include

ifneq ($(shell test -f $(PREFIX)/share/config.site && echo 1),1)
  $(error Error: $(PREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

DEPENDS = \
	pcre expat gettext sqlite3 libgpg-error \
	libgcrypt bzip2 liblzo2 libzip freetype2 fontconfig \
	openssl gmp nettle gnutls curl \
	libjpeg-turbo libpng fribidi libass \
	librtmp libxml2 yajl libmicrohttpd mysql libffi \
	python27 libshairplay \
	libplist libcec libbluray tinyxml dummy-libxbmc \
	libamplayer libssh taglib libusb libnfs \
	pythonmodule-pil pythonmodule-setuptools \
	libxslt ffmpeg platform crossguid \
        libdvdread libdvdnav libdvdcss


FFMPEG_DEPENDS = gnutls

ifeq ($(ENABLE_GPLV3),yes)
  DEPENDS+=samba-gplv3 libcdio-gplv3
else
  DEPENDS+=samba libcdio
endif

ifeq ($(OS),ios)
  EXCLUDED_DEPENDS = libcec libusb libdvdcss
  ifeq ($(TARGET_PLATFORM),appletvos)
    DEPENDS += boblight
  endif
else
  DVDREAD_DEPS = libdvdcss
endif

ifeq ($(OS),osx)
  DEPENDS += libsdl
  EXCLUDED_DEPENDS = libusb
endif

ifeq ($(OS),android)
  DEPENDS += mdnsresponder libuuid
  CROSSGUID_DEPS = libuuid
endif

DEPENDS := $(filter-out $(EXCLUDED_DEPENDS),$(DEPENDS))

ZLIB=
ifneq ($(HAS_ZLIB),1)
  DEPENDS += zlib
  ZLIB = zlib
endif

ICONV=
ifeq ($(NEED_LIBICONV),1)
  DEPENDS += libiconv
  ICONV = libiconv
endif

ALSA_LIB=
LINUX_SYSTEM_LIBS=
ifeq ($(OS),linux)
  DEPENDS += libuuid
  #not for raspberry pi
  ifneq ($(TARGET_PLATFORM),raspberry-pi)
    DEPENDS += libsdl2 linux-system-libs
    LINUX_SYSTEM_LIBS = linux-system-libs
  endif
  DEPENDS += alsa-lib
  ALSA_LIB = alsa-lib
  CROSSGUID_DEPS = libuuid
endif

.PHONY: $(DEPENDS)

all: .installed-$(PLATFORM)

gettext: $(ICONV)
libgcrypt: libgpg-error
fontconfig: freetype2 expat $(ICONV)
curl: openssl
libass: fontconfig libpng freetype2 expat $(ICONV)
librtmp: openssl
libmicrohttpd: gnutls libgcrypt libgpg-error
python27: expat gettext libxml2 sqlite3 openssl libffi
libcdio: $(ICONV)
libplist: libxml2 $(ZLIB)
libbluray: $(ICONV) libxml2
libssh: libgcrypt openssl $(ZLIB)
mysql: openssl $(ZLIB)
libzip: $(ZLIB)
libpng: $(ZLIB)
openssl: $(ZLIB)
gnutls: nettle $(ZLIB)
nettle: gmp
pythonmodule-pil: $(ZLIB) libjpeg-turbo libpng freetype2 python27 pythonmodule-setuptools
pythonmodule-setuptools: python27
libsdl2: $(LINUX_SYSTEM_LIBS)
libxslt: libgcrypt libxml2
ffmpeg: $(ICONV) $(ZLIB) bzip2 $(FFMPEG_DEPENDS)
libcec: platform
crossguid: $(CROSSGUID_DEPS)
libdvdnav: libdvdread
libdvdread: $(DVDREAD_DEPS)

.installed-$(PLATFORM): $(DEPENDS)
	touch $@
	@echo "Dependencies built successfully."

$(DEPENDS):
	$(MAKE) -C $@

clean:
	for d in $(DEPENDS); do $(MAKE) -C $$d clean; done

# Debug target, this will DELETE all data in staging!
test-dependencies:
	( for d in $(DEPENDS); do \
	rm -rf $(PREFIX); \
	mkdir -p $(PREFIX)/include $(PREFIX)/share $(PREFIX)/bin; \
	cp -f config.site Toolchain.cmake $(PREFIX)/share/; \
	$(MAKE) distclean; \
	$(MAKE) $$d; done ) && echo "$@ built successfully"


distclean::
	for d in $(DEPENDS); do $(MAKE) -C $$d distclean; done

linux-system-libs-egl:
	[ -f $(PREFIX)/lib/pkgconfig/egl.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/egl.pc $(PREFIX)/lib/pkgconfig/egl.pc
	[ -f $(PREFIX)/lib/pkgconfig/damageproto.pc ] || ln -s  /usr/share/pkgconfig/damageproto.pc $(PREFIX)/lib/pkgconfig/damageproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/fixesproto.pc ] || ln -s  /usr/share/pkgconfig/fixesproto.pc $(PREFIX)/lib/pkgconfig/fixesproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/x11-xcb.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/x11-xcb.pc $(PREFIX)/lib/pkgconfig/x11-xcb.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-dri2.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-dri2.pc $(PREFIX)/lib/pkgconfig/xcb-dri2.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-dri3.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-dri3.pc $(PREFIX)/lib/pkgconfig/xcb-dri3.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-glx.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-glx.pc $(PREFIX)/lib/pkgconfig/xcb-glx.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-xfixes.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-xfixes.pc $(PREFIX)/lib/pkgconfig/xcb-xfixes.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-present.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-present.pc $(PREFIX)/lib/pkgconfig/xcb-present.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-randr.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xcb-randr.pc $(PREFIX)/lib/pkgconfig/xcb-randr.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-render.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xcb-render.pc $(PREFIX)/lib/pkgconfig/xcb-render.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-shape.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-shape.pc $(PREFIX)/lib/pkgconfig/xcb-shape.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb-sync.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xcb-sync.pc $(PREFIX)/lib/pkgconfig/xcb-sync.pc
	[ -f $(PREFIX)/lib/pkgconfig/xdamage.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xdamage.pc $(PREFIX)/lib/pkgconfig/xdamage.pc
	[ -f $(PREFIX)/lib/pkgconfig/xf86vidmodeproto.pc ] || ln -s /usr/share/pkgconfig/xf86vidmodeproto.pc $(PREFIX)/lib/pkgconfig/xf86vidmodeproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/xfixes.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xfixes.pc $(PREFIX)/lib/pkgconfig/xfixes.pc
	[ -f $(PREFIX)/lib/pkgconfig/xshmfence.pc ] || ln -s  /usr/lib/$(HOST)/pkgconfig/xshmfence.pc $(PREFIX)/lib/pkgconfig/xshmfence.pc
	[ -f $(PREFIX)/lib/pkgconfig/xxf86vm.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xxf86vm.pc $(PREFIX)/lib/pkgconfig/xxf86vm.pc

linux-system-libs: linux-system-libs-egl
	[ -f $(PREFIX)/lib/pkgconfig/x11.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/x11.pc $(PREFIX)/lib/pkgconfig/x11.pc
	[ -f $(PREFIX)/lib/pkgconfig/xproto.pc ] || ln -s /usr/share/pkgconfig/xproto.pc $(PREFIX)/lib/pkgconfig/xproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/kbproto.pc ] || ln -s /usr/share/pkgconfig/kbproto.pc $(PREFIX)/lib/pkgconfig/kbproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/xcb.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xcb.pc $(PREFIX)/lib/pkgconfig/xcb.pc
	[ -f $(PREFIX)/lib/pkgconfig/pthread-stubs.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/pthread-stubs.pc $(PREFIX)/lib/pkgconfig/pthread-stubs.pc
	[ -f $(PREFIX)/lib/pkgconfig/xau.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xau.pc $(PREFIX)/lib/pkgconfig/xau.pc
	[ -f $(PREFIX)/lib/pkgconfig/xdmcp.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xdmcp.pc $(PREFIX)/lib/pkgconfig/xdmcp.pc
	[ -f $(PREFIX)/lib/pkgconfig/xext.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xext.pc $(PREFIX)/lib/pkgconfig/xext.pc
	[ -f $(PREFIX)/lib/pkgconfig/xextproto.pc ] || ln -s /usr/share/pkgconfig/xextproto.pc $(PREFIX)/lib/pkgconfig/xextproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/xrandr.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xrandr.pc $(PREFIX)/lib/pkgconfig/xrandr.pc
	[ -f $(PREFIX)/lib/pkgconfig/xrender.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xrender.pc $(PREFIX)/lib/pkgconfig/xrender.pc
	[ -f $(PREFIX)/lib/pkgconfig/randrproto.pc ] || ln -s /usr/share/pkgconfig/randrproto.pc $(PREFIX)/lib/pkgconfig/randrproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/renderproto.pc ] || ln -s /usr/share/pkgconfig/renderproto.pc $(PREFIX)/lib/pkgconfig/renderproto.pc
	[ -f $(PREFIX)/lib/pkgconfig/xt.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xt.pc $(PREFIX)/lib/pkgconfig/xt.pc
	[ -f $(PREFIX)/lib/pkgconfig/ice.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/ice.pc $(PREFIX)/lib/pkgconfig/ice.pc
	[ -f $(PREFIX)/lib/pkgconfig/sm.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/sm.pc $(PREFIX)/lib/pkgconfig/sm.pc
	[ -f $(PREFIX)/lib/pkgconfig/xmu.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/xmu.pc $(PREFIX)/lib/pkgconfig/xmu.pc
	[ -f $(PREFIX)/lib/pkgconfig/libdrm.pc ] || ln -s /usr/lib/$(HOST)/pkgconfig/libdrm.pc $(PREFIX)/lib/pkgconfig/libdrm.pc
