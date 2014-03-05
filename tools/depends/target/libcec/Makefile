include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libcec
VERSION=2.1.4
SOURCE=$(LIBNAME)-$(VERSION)-2
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-rpi \

LIBDYLIB=$(PLATFORM)/src/lib/.libs/libcec.la

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/src/lib install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libcec.la $(PREFIX)/lib/libcec.so $(PREFIX)/lib/libcec.so.2
	mv -f $(PREFIX)/lib/libcec.so.2.0.1 $(PREFIX)/lib/libcec.so
	$(RPL) -e "libcec.so.2" "libcec.so\x00\x00" $(PREFIX)/lib/libcec.so
	-$(READELF) --dynamic $(PREFIX)/lib/libcec.so | grep ibrary
endif
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

