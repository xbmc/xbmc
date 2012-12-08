include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=libcec
VERSION=2.0.5-3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
          ac_cv_search_dlopen=yes

LIBDYLIB=$(PLATFORM)/src/lib/.libs/libcec.so.2.0.0

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/src/lib install
	rm -f $(PREFIX)/lib/libcec.la $(PREFIX)/lib/libcec.so $(PREFIX)/lib/libcec.so.2
	mv -f $(PREFIX)/lib/libcec.so.2.0.0 $(PREFIX)/lib/libcec.so
	$(RPL) -e "libcec.so.2" "libcec.so\x00\x00" $(PREFIX)/lib/libcec.so
	-$(READELF) --dynamic $(PREFIX)/lib/libcec.so | grep ibrary
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

