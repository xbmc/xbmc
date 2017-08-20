include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile readelfOnly.patch

# lib name, version
LIBNAME=binutils
VERSION=2.29
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CONFIGURE=./configure --prefix=$(NATIVEPREFIX)

LIBDYLIB=$(PLATFORM)/binutils/readelf

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../readelfOnly.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) all-binutils

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/binutils; $(MAKE) install-binPROGRAMS
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
