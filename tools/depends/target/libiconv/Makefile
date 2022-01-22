include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libiconv
VERSION=1.15
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=1233fe3ca09341b53354fd4bfe342a7589181145a1232c9919583a8c9979636855839049f3406f253a9d9829908816bb71fd6d34dd544ba290d6f04251376b1a
include ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) build-aux/; \
          cp -f $(CONFIG_SUB) $(CONFIG_GUESS) libcharset/build-aux/; \
          ./configure --prefix=$(PREFIX) --disable-shared --enable-extra-encodings

LIBDYLIB=$(PLATFORM)/lib/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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

