include ../../Makefile.include LIBFSTRCMP-VERSION
DEPS= ../../Makefile.include Makefile LIBFSTRCMP-VERSION

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
