include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=libmpeg2
VERSION=0.5.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .auto/ ; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) --disable-sdl

LIBDYLIB=$(PLATFORM)/$(LIBNAME)/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/lib/libmpeg2.la $(PREFIX)/lib/libmpeg2.so $(PREFIX)/lib/libmpeg2.so.0
	mv -f $(PREFIX)/lib/libmpeg2.so.0.1.0 $(PREFIX)/lib/libxbmpeg2.so
	ln -sf $(PREFIX)/lib/libxbmpeg2.so $(PREFIX)/lib/libmpeg2.so
	$(RPL) -e "libmpeg2.so.0" "libxbmpeg2.so" $(PREFIX)/lib/libxbmpeg2.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbmpeg2.so | grep ibrary
	#
	rm -f $(PREFIX)/lib/libmpeg2convert.la $(PREFIX)/lib/libmpeg2convert.so $(PREFIX)/lib/libmpeg2convert.so.0
	mv -f $(PREFIX)/lib/libmpeg2convert.so.0.0.0 $(PREFIX)/lib/libxbmpeg2convert.so
	ln -sf $(PREFIX)/lib/libxbmpeg2convert.so $(PREFIX)/lib/libmpeg2convert.so
	$(RPL) -e "libmpeg2convert.so.0" "libxbmpeg2convert.so" $(PREFIX)/lib/libxbmpeg2convert.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbmpeg2convert.so | grep ibrary
	#
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

