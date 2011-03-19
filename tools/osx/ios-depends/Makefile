BUILDDIR=/Users/Shared/xbmc-depends/ios-4.2_arm7

SUBDIRS = \
	Backrow \
	gas-preprocessor help2man autoconf automake pkg-config tar dpkg \
	pcre expat gettext readline sqlite3 \
	liblzo liblzo2 freetype2 fontconfig \
	openssl libssh2 curl \
	jpeg tiff libpng libogg \
	libvorbis libmad fribidi libwavpack libmpeg2 \
	faad2 libass libcdio libsamplerate librtmp \
	libmicrohttpd libmodplug libbluray \
	samba python26 mysqlclient boost

.PHONY: setup $(SUBDIRS)

all: setup $(SUBDIRS)

setup:
	mkdir -p $(BUILDDIR)/bin
	mkdir -p $(BUILDDIR)/lib
	mkdir -p $(BUILDDIR)/slib
	mkdir -p $(BUILDDIR)/share
	mkdir -p $(BUILDDIR)/include

$(SUBDIRS):
	$(MAKE) -C $@

fontconfig: freetype2
libmad: gas-preprocessor

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d distclean; done
	rm -rf build
	rm -rf $(BUILDDIR)
