include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-patch-autoconf-GH-5130.patch ../../download-files.include

# lib name, version
LIBNAME=curl
VERSION=7.68.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=ad7390fd700cb74db356a39e842dab011823b87d4047687f2a8c2e0f2920a4f8c6c193ba56391489a75939cc5c39a4dccec4e4ceeac516eb7394f03e0fb7aeae
include ../../download-files.include
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --disable-ldap \
          --without-libssh2 --disable-ntlm-wb --enable-ipv6 --without-librtmp \
          --without-libidn2 --with-ca-fallback --with-ssl=$(PREFIX) --with-nghttp2=$(PREFIX)

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-patch-autoconf-GH-5130.patch
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
