include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=curl
VERSION=7.68.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --disable-ldap \
          --without-libssh2 --disable-ntlm-wb --enable-ipv6 --without-librtmp \
          --without-libidn2 --with-ca-fallback --with-ssl=$(PREFIX) --with-nghttp2=$(PREFIX)

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
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
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

.PHONY: update-cacert
update-cacert: $(PLATFORM)
	$(PLATFORM)/lib/mk-ca-bundle.pl -u $(CMAKE_SOURCE_DIR)/system/certs/cacert.pem
