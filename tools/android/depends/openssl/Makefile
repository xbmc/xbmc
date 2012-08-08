include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=openssl
VERSION=0.9.8r
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=MACHINE=$(HOST) ./config no-shared zlib no-shared --openssldir=$(PREFIX)

LIBDYLIB=$(PLATFORM)/libssl.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	#when compiled on darwin it just won't realise that we do crosscompiling
	#so it would stick in -arch i386 or -arch x86_64 into the cflags
	#that would break the cross compile so we have to get rid of these
	sed -ie 's/-arch i386//' $(PLATFORM)/Configure
	sed -ie 's/-arch x86_64//' $(PLATFORM)/Configure
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install_sw
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

