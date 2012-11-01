include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=curl
VERSION=7.21.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
CFLAGS+=-DANDROID

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) --without-libssh2 \
ac_cv_file__dev_urandom=yes \

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

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
	rm -f $(PREFIX)/lib/libcurl.la $(PREFIX)/lib/libcurl.so $(PREFIX)/lib/libcurl.so.5
	mv -f $(PREFIX)/lib/libcurl.so.5.2.0 $(PREFIX)/lib/libcurl.so
	$(RPL) -e "libcurl.so.5" "libcurl.so\x00\x00" $(PREFIX)/lib/libcurl.so
	-$(READELF) --dynamic $(PREFIX)/lib/libcurl.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

