include ../../Makefile.include CURL-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CURL-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DBUILD_CURL_EXE=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DCURL_DISABLE_TESTS=ON \
              -DUSE_MANUAL=OFF \
              -DCURL_DISABLE_LDAP=ON \
              -DCURL_DISABLE_LDAPS=ON \
              -DCURL_DISABLE_VERBOSE_STRINGS=ON \
              -DENABLE_IPV6=ON \
              -DUSE_LIBIDN2=OFF \
              -DCURL_USE_LIBSSH2=OFF \
              -DCURL_USE_OPENSSL=ON \
              -DCURL_ENABLE_SSL=ON \
              -DUSE_NGHTTP2=ON \
              -DCURL_CA_FALLBACK=ON

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

.PHONY: update-cacert
update-cacert: $(PLATFORM)
	$(PLATFORM)/lib/mk-ca-bundle.pl -u $(CMAKE_SOURCE_DIR)/system/certs/cacert.pem
