include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=libmodplug
VERSION=0.8.7
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST)

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).so

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
	rm -f $(PREFIX)/lib/libmodplug.la $(PREFIX)/lib/libmodplug.so $(PREFIX)/lib/libmodplug.so.0
	mv -f $(PREFIX)/lib/libmodplug.so.0.0.0 $(PREFIX)/lib/libmodplug.so
	$(RPL) -e "libmodplug.so.0" "libmodplug.so\x00\x00" $(PREFIX)/lib/libmodplug.so
	-$(READELF) --dynamic $(PREFIX)/lib/libmodplug.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

