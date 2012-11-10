include ../Makefile.include
include ../config.site.mk

XBMC_ADDONSDIR=../../../../addons

# lib name, version
LIBNAME=xbmc-pvr-addons
VERSION=ef09a31abc3e67778d91583dd05ce8455865b2d6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-addons-with-dependencies

LIBDYLIB=$(SOURCE)/addons/pvr.demo/.libs/libpvrdemo-addon.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	git clone git://github.com/opdenkamp/xbmc-pvr-addons.git $(SOURCE)
	cd $(SOURCE); git archive --format=tar --prefix=$(SOURCE)/ $(VERSION) | gzip -9 > $(TARBALLS_LOCATION)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	for ADDON in `find $(SOURCE)/addons -type d -name "pvr.*"`; do \
	  ADDON=`basename $$ADDON` ; \
	  mkdir -p $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -PRf $(SOURCE)/addons/$$ADDON/addon/* $(XBMC_ADDONSDIR)/$$ADDON ; \
	  cp -Pf $(SOURCE)/addons/$$ADDON/*.pvr $(XBMC_ADDONSDIR)/$$ADDON ; \
	  install_name_tool -id "$$ADDON" `find "$(XBMC_ADDONSDIR)/$$ADDON" -type f -name "*.pvr"` ; \
	done
	touch .installed

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
	rm -f $(TARBALLS_LOCATION)/$(ARCHIVE)

