include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=swig
VERSION=4.0.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=05e7da70ce6d9a733b96c0bcfa3c1b82765bd859f48c74759bbf4bb1467acb1809caa310cba5e2b3280cd704fca249eaa0624821dffae1d2a75097c7f55d14ed
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --without-alllang --without-x --disable-ccache

LIBDYLIB=$(PLATFORM)/swig

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

