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
	libass \
	libmodplug librtmp libxml2 yajl libmicrohttpd mysql libffi \
	python26 afpfs-ng libshairplay \
	libplist libcec libbluray boost tinyxml dummy-libxbmc \
	libamplayer libssh taglib xbmc-pvr-addons libusb libnfs libmp3lame \
	pythonmodule-pil libxslt ffmpeg

ifeq ($(ENABLE_GPLV3),1)
  DEPENDS+=samba-gplv3 libcdio-gplv3
else
  DEPENDS+=samba libcdio
endif

ifeq ($(OS),ios)
  DEPENDS += Backrow
  EXCLUDED_DEPENDS = libcec libcrystalhd libusb gmp nettle gnutls
endif

ifeq ($(OS),osx)
  DEPENDS += libGLEW libsdl libcrystalhd
  EXCLUDED_DEPENDS = libusb gmp nettle gnutls
endif

ifeq ($(OS),android)
  DEPENDS += mdnsresponder android-sources-ics
  EXCLUDED_DEPENDS = gmp nettle gnutls
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
    DEPENDS += alsa-lib libsdl linux-system-libs
    ALSA_LIB = alsa-lib
    LINUX_SYSTEM_LIBS = linux-system-libs
  endif
  FFMPEG_DEPENDS = gnutls
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
libmicrohttpd: openssl libgpg-error libgcrypt
python26: expat gettext libxml2 sqlite3 openssl libffi
libcdio: $(ICONV)
afpfs-ng: libgcrypt $(ICONV)
libplist: libxml2 $(ZLIB)
libbluray: $(ICONV) libxml2
libssh: openssl
xbmc-pvr-addons: boost mysql
mysql: openssl
libzip: $(ZLIB)
libmp3lame: $(ICONV)
libpng: $(ZLIB)
openssl: $(ZLIB)
gnutls: nettle $(ZLIB)
nettle: gmp
pythonmodule-pil: $(ZLIB) libjpeg-turbo libpng freetype2 python26
libsdl: $(LINUX_SYSTEM_LIBS)
libxslt: libgcrypt
ffmpeg: $(ICONV) $(ZLIB) bzip2 libvorbis $(FFMPEG_DEPENDS)

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


distclean:
	for d in $(DEPENDS); do $(MAKE) -C $$d distclean; done

SYSTEM_LIBS=x11 xproto kbproto xcb pthread-stubs xau xdmcp xext xextproto xt ice sm xmu

linux-system-libs:
	for p in $(shell pkg-config --variable pc_path pkg-config | tr ':' ' '); do \
	  for l in $(SYSTEM_LIBS); do \
	    if [ -e $$p/$$l.pc ]; then cp $$p/$$l.pc $(PREFIX)/lib/pkgconfig/$$l.pc;fi \
	  done; \
	done
