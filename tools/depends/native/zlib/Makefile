include ../../Makefile.include ZLIB-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include ZLIB-VERSION Makefile ../../download-files.include

# configuration settings
CONFIGURE= CC="$(CC_FOR_BUILD)" CFLAGS="$(NATIVE_CFLAGS)" ./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
