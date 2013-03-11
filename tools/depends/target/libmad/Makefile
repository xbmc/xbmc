include ../../Makefile.include
DEPS= ../../Makefile.include 01-libmad-pkgconfig.patch Makefile

# lib name, version
LIBNAME=libmad
VERSION=0.15.1b
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=rm config.sub config.guess; $(AUTOMAKE) -ac; ./configure --prefix=$(PREFIX) --enable-fpm=default

LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); touch NEWS AUTHORS ChangeLog
	cd $(PLATFORM); patch -p1 < ../01-libmad-pkgconfig.patch
	cd $(PLATFORM); patch -p1 < ../02-libmad-thumb-fix.patch
	sed -ie "s|libmad.list|libmad.list mad.pc|g" "$(PLATFORM)/configure.ac"
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); sed -i '/-fforce-mem/d' configure; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libmad.la $(PREFIX)/lib/libmad.so $(PREFIX)/lib/libmad.so.0
	mv -f $(PREFIX)/lib/libmad.so.0.2.1 $(PREFIX)/lib/libmad.so
	$(RPL) -e "libmad.so.0" "libmad.so\x00\x00" $(PREFIX)/lib/libmad.so
	-$(READELF) --dynamic $(PREFIX)/lib/libmad.so | grep ibrary
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

