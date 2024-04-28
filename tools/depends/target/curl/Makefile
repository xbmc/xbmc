include ../../Makefile.include CURL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CURL-VERSION ../../download-files.include

CMAKE_OPTIONS=-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DCMAKE_BUILD_TYPE=Release \
              -DBUILD_CURL_EXE=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DBUILD_STATIC_LIBS=ON \
              -DBUILD_LIBCURL_DOCS=OFF \
              -DENABLE_CURL_MANUAL=OFF \
              -DCURL_DISABLE_TESTS=OFF \
              -DCURL_DISABLE_LDAP=ON \
              -DCURL_DISABLE_LDAPS=ON \
              -DCURL_DISABLE_SMB=OFF \
              -DCURL_USE_OPENSSL=ON \
              -DOPENSSL_ROOT_DIR=$(PREFIX) \
              -DCURL_BROTLI=ON \
              -DUSE_NGHTTP2=ON \
              -DUSE_LIBIDN2=OFF \
              -DCURL_USE_LIBSSH2=OFF \
              -DCURL_USE_GSSAPI=OFF \
              -DCURL_CA_FALLBACK=ON

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) ${CMAKE_OPTIONS} ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

.PHONY: update-cacert
update-cacert: $(PLATFORM)
	$(PLATFORM)/scripts/mk-ca-bundle.pl -u $(CMAKE_SOURCE_DIR)/system/certs/cacert.pem
