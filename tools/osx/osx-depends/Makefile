BUILDDIR=/Users/Shared/xbmc-depends/osx-10.4_i386

SUBDIRS = \
	help2man autoconf automake libtool pkg-config yasm cmake \
	pcre expat gettext readline sqlite3 \
	liblzo liblzo2 freetype2 fontconfig \
	openssl libssh2 curl \
	jpeg tiff libpng libogg \
	libvorbis libflac libmad fribidi libwavpack libmpeg2 \
	faad2 libass libcdio libsamplerate librtmp libcrystalhd \
	libGLEW libsdl libsdl_mixer libsdl_image \
	libmicrohttpd libmodplug libbluray \
	samba python26 mysqlclient boost \

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

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d distclean; done
	rm -rf $(BUILDDIR)
