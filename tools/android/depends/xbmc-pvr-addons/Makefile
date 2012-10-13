include ../Makefile.include
DEPS= ../Makefile.include Makefile

XBMC_ADDONSDIR=../../../../addons

# lib name, version
LIBNAME=xbmc-pvr-addons
VERSION=a6693d02b670a5f0a35c7e41d62250869a9e76b7
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)

LIBDYLIB=$(PLATFORM)/addons/pvr.demo/.libs/libpvrdemo-addon.dylib

all: $(LIBDYLIB) .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	git clone git://github.com/opdenkamp/xbmc-pvr-addons.git $(PLATFORM)
	cd $(PLATFORM); git archive --format=tar --prefix=$(SOURCE)/ $(VERSION) | gzip -9 > $(TARBALLS_LOCATION)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); ./bootstrap
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	make -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	for ADDON in `find $(PLATFORM)/addons -type d -name "pvr.*"`; do \
	  ADDON=`basename $$ADDON` ; \
	  mkdir -p $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -PRf $(PLATFORM)/addons/$$ADDON/addon/* $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -Pf $(PLATFORM)/addons/$$ADDON/*.pvr $(XBMC_ADDONSDIR)/$$ADDON ; \
	done
	make -C $(PLATFORM) install
	touch .installed-$(PLATFORM)

clean:
	make -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -f $(TARBALLS_LOCATION)/$(ARCHIVE)

