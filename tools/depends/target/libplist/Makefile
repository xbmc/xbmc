include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-disable-tools-tests.patch

# lib name, version
LIBNAME=libplist
VERSION=2.2.0
SOURCE=$(LIBNAME)-$(VERSION)
FILENAME=v$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
LIBDYLIB=$(PLATFORM)/src/.libs/libplist++-2.0.a
CFLAGS+=-fvisibility=default
all: .installed-$(PLATFORM)

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; ./configure --prefix=$(PREFIX) --disable-shared --without-cython

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-tools-tests.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
