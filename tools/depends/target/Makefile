include ../Makefile.include

ifneq ($(shell test -f $(PREFIX)/share/config.site && echo 1),1)
  $(error Error: $(PREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

DEPENDS = \
	pcre expat gettext sqlite3 libgpg-error \
	libgcrypt bzip2 liblzo2 libzip freetype2 fontconfig \
	openssl gmp nettle gnutls libssh2 curl \
	libjpeg-turbo tiff jasper libpng \
	libogg libvorbis libflac fribidi libmpeg2 \
	libass libsquish \
	libmodplug librtmp libxml2 yajl libmicrohttpd mysql libffi \
	python26 libshairplay \
	libplist libcec libbluray boost tinyxml dummy-libxbmc \
	libamplayer libssh taglib libusb libnfs libmp3lame \
	pythonmodule-pil libxslt ffmpeg platform

FFMPEG_DEPENDS = gnutls

ifeq ($(ENABLE_GPLV3),yes)
  DEPENDS+=samba-gplv3 libcdio-gplv3
else
  DEPENDS+=samba libcdio
endif

ifeq ($(OS),ios)
  DEPENDS += Backrow
  EXCLUDED_DEPENDS = libcec libusb
endif

ifeq ($(OS),osx)
  DEPENDS += libGLEW libsdl
  EXCLUDED_DEPENDS = libusb
endif

ifeq ($(OS),android)
  DEPENDS += mdnsresponder android-sources-ics google-breakpad
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
  #not for raspberry pi
  ifneq ($(TARGET_PLATFORM),raspberry-pi)
    DEPENDS += libsdl2 linux-system-libs
    LINUX_SYSTEM_LIBS = linux-system-libs
  endif
  DEPENDS += alsa-lib
  ALSA_LIB = alsa-lib
endif

.PHONY: $(DEPENDS)

all: .installed-$(PLATFORM)

gettext: $(ICONV)
libgcrypt: libgpg-error
fontconfig: freetype2 expat $(ICONV)
libssh2: libgcrypt openssl
curl: openssl libssh2
tiff: libjpeg-turbo
jasper: libjpeg-turbo
libvorbis: libogg
libflac: libogg gettext
libass: fontconfig libpng freetype2 expat $(ICONV)
librtmp: openssl
libmicrohttpd: gnutls libgcrypt libgpg-error
python26: expat gettext libxml2 sqlite3 openssl libffi
libcdio: $(ICONV)
libplist: libxml2 $(ZLIB)
libbluray: $(ICONV) libxml2
libssh: openssl
mysql: openssl
libzip: $(ZLIB)
libmp3lame: $(ICONV)
libpng: $(ZLIB)
openssl: $(ZLIB)
gnutls: nettle $(ZLIB)
nettle: gmp
pythonmodule-pil: $(ZLIB) libjpeg-turbo libpng freetype2 python26
libsdl2: $(LINUX_SYSTEM_LIBS)
libxslt: libgcrypt
ffmpeg: $(ICONV) $(ZLIB) bzip2 libvorbis $(FFMPEG_DEPENDS)
libcec: platform

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

linux-system-libs:
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
