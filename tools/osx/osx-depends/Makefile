
SUBDIRS = \
	help2man autoconf automake pkg-config yasm cmake \
	pcre expat gettext readline sqlite3 \
	liblzo liblzo2 freetype2 fontconfig \
	openssl libssh2 curl \
	jpeg tiff libpng libogg \
	libvorbis libmad fribidi libwavpack libmpeg2 \
	faad2 libcdio libsamplerate librtmp \
	libGLEW libsdl libsdl_mixer libsdl_image \
	samba python26 mysqlclient boost \

.PHONY: subdirs $(SUBDIRS)

all: $(SUBDIRS)

subdirs:
	mkdir -p $(CURDIR)/build/bin
	mkdir -p $(CURDIR)/build/lib
	mkdir -p $(CURDIR)/build/slib
	mkdir -p $(CURDIR)/build/include
	$(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean::
	for d in $(SUBDIRS); do $(MAKE) -C $$d distclean; done
	rm -rf build
