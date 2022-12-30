include ../../Makefile.include FSTRCMP-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile FSTRCMP-VERSION ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/lib/.libs/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); autoreconf -vif; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) lib/libfstrcmp.la

.installed-$(PLATFORM): $(LIBDYLIB)
	mkdir -p $(PREFIX)/lib/pkgconfig
	$(MAKE) -C $(PLATFORM) install-include install-libdir
	rm -f $(PREFIX)/lib/libfstrcmp.so*
	rm -f $(PREFIX)/lib/libfstrcmp.dylib*
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
