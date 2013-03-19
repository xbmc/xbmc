include ../Makefile.include

ifneq ($(shell test -f $(PREFIX)/share/config.site && echo 1),1)
  $(error Error: $(PREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

DEPENDS = \
	pcre expat gettext sqlite3 libgpg-error \
	libgcrypt bzip2 liblzo2 libzip freetype2 fontconfig \
	openssl libssh2 curl \
	libjpeg-turbo tiff jasper libpng \
	libogg libvorbis libflac libmad fribidi libmpeg2 \
	libass libsamplerate \
	libmodplug librtmp libxml2 yajl libmicrohttpd mysql libffi \
	python26 afpfs-ng libshairport \
	libplist libcec libbluray boost tinyxml dummy-libxbmc \
	libamplayer libssh taglib xbmc-pvr-addons libusb libnfs libmp3lame \
        pythonmodule-pil

ifeq ($(ENABLE_GPLV3),1)
  DEPENDS+=samba-gplv3 libcdio-gplv3
else
  DEPENDS+=samba libcdio
endif

ifeq ($(OS),ios)
  DEPENDS += Backrow
  EXCLUDED_DEPENDS = libcec libcrystalhd libusb
endif

ifeq ($(OS),osx)
  DEPENDS += libGLEW libsdl libcrystalhd
  EXCLUDED_DEPENDS = libusb
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
ifeq ($(OS),linux)
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
libxml2: $(ICONV)
libmicrohttpd: openssl libgpg-error libgcrypt
python26: expat gettext libxml2 sqlite3 openssl libffi
libcdio: $(ICONV)
afpfs-ng: libgcrypt $(ICONV)
libshairport: openssl
libplist: libxml2 $(ZLIB)
libbluray: $(ICONV) libxml2
libssh: openssl
xbmc-pvr-addons: boost mysql
mysql: openssl
libzip: $(ZLIB)
libmp3lame: $(ICONV)
libpng: $(ZLIB)
openssl: $(ZLIB)
pythonmodule-pil: $(ZLIB) libjpeg-turbo libpng freetype2

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

