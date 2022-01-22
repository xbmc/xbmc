include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=nghttp2
VERSION=1.46.0
ARCHIVE=$(LIBNAME)-$(VERSION).tar.xz
SHA512=fcf3573bcc421705190c7cf0e3230f6f3028b669cb2976d29cfeb73e706deaae91ce60d0a615472e3f296454049ea5798f1e8defdd260a98895e94fea6a7a16b
include ../../download-files.include

# configuration settings
# no extra libs are required when using --enable-lib-only
CONFIGURE=./configure --prefix=$(PREFIX) --enable-lib-only --disable-shared \
            --without-jansson \
            --without-libevent-openssl \
            --without-libcares \
            --without-libev \
            --without-cunit \
            --without-openssl \
            --without-zlib

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

