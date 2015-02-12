include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=curl
ifeq (osx, $(findstring osx, $(OS)))
VERSION=7.21.6
else
VERSION=7.39.0
endif
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --without-libssh2 \

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq (osx, $(findstring osx, $(OS)))
	cd $(PLATFORM); patch -p1 < ../0001-HTTP-reset-expected-DL-UL-sizes-on-redirects.patch
endif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	# install only libcurl and includes
	$(MAKE) -C $(PLATFORM)/lib install
	$(MAKE) -C $(PLATFORM)/include install
	# make sure to also install libcurl.pc, we need to do
	# this because we are selectively installing only libcurl
	$(MAKE) -C $(PLATFORM) install-pkgconfigDATA
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libcurl.la $(PREFIX)/lib/libcurl.so $(PREFIX)/lib/libcurl.so.5
	mv -f $(PREFIX)/lib/libcurl.so.5.3.0 $(PREFIX)/lib/libcurl.so
	$(RPL) -e "libcurl.so.5" "libcurl.so\x00\x00" $(PREFIX)/lib/libcurl.so
	-$(READELF) --dynamic $(PREFIX)/lib/libcurl.so | grep ibrary
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

