include ../../Makefile.include CURL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CURL-VERSION ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
                      --disable-shared \
                      --disable-docs \
                      --disable-ldap \
                      --without-libssh2 \
                      --disable-ntlm-wb \
                      --enable-ipv6 \
                      --without-librtmp \
                      --without-libidn2 \
                      --with-ca-fallback \
                      --with-ssl=$(PREFIX) \
                      --with-brotli=$(PREFIX) \
                      --with-nghttp2=$(PREFIX) \
                      --with-zlib \
                      --without-libpsl \
                      --without-zstd \
                      --without-gssapi \
                      --without-gsasl \
                      --without-hyper \
                      --without-ngtcp2 \
                      --without-nghttp3 \
                      --without-quiche \
                      --without-msh3 \
                      --without-gnutls \
                      --without-nss \
                      --without-mbedtls \
                      --without-wolfssl

LIBDYLIB=$(PLATFORM)/lib/.libs/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
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
