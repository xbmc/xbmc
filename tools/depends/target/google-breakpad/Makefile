include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=google-breakpad
VERSION=1434
ARCHIVE=$(LIBNAME)-$(VERSION).tar.bz2

LIBDYLIB=$(PLATFORM)/src/client/linux/libbreakpad_client.a

# configuration settings
CONFIGURE= ./configure --prefix=$(PREFIX)  \
  --disable-processor --disable-tools

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	cp -Rf $(PLATFORM)/src/common/android/include/* $(PREFIX)/include/breakpad/
	mkdir -p $(PREFIX)/include/breakpad/client/linux/dump_writer_common
	cp -f $(PLATFORM)/src/client/linux/dump_writer_common/*.h $(PREFIX)/include/breakpad/client/linux/dump_writer_common/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

