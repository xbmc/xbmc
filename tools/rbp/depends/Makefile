include ../Makefile.include

ifeq ($(USE_BUILDROOT),1)
	BUILDTOOLS = 
	SUBDIRS = ffmpeg xbmc
else
	BUILDTOOLS = help2man autoconf automake libtool pkg-config yasm cmake patchelf
	SUBDIRS = liblzo2 tiff libnfs jasper libplist libshairplay tinyxml ffmpeg xbmc
endif

IMAGENAME = xbmc-rbp-`date +'%y%m%d'`

.PHONY: buildtools subdirs

all: buildtools subdirs

buildtools:
	for d in $(BUILDTOOLS); do $(MAKE) -C $$d; done

subdirs:
	for d in $(SUBDIRS); do if test -f "$$d/.installed"; then :; else $(MAKE) -C $$d || exit 1; fi; done

clean:
	for d in $(BUILDTOOLS); do $(MAKE) -C $$d clean; done
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done

distclean::
	for d in $(BUILDTOOLS); do $(MAKE) -C $$d distclean; done
	for d in $(SUBDIRS); do $(MAKE) -C $$d distclean; done
	rm -rf $(XBMCPREFIX)

image:
	make -C $(BUILDROOT) tarroot IMAGE=$(CURDIR)/$(IMAGENAME) TAR_COMPRESSOR=
	echo "S111xbmc S110rc.local" | cat $(TARGETFS)/etc/init.d/rc.deps - > $(CURDIR)/packaging/etc/init.d/rc.deps
	cd packaging; tar -uf $(CURDIR)/$(IMAGENAME).tar *
	tar -uf $(CURDIR)/$(IMAGENAME).tar $(XBMCPREFIX) 
	gzip $(IMAGENAME).tar
