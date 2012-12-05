include ../Makefile.include
DEPS= ../Makefile.include fix-android-include.patch Makefile

# lib name, version
LIBNAME=libmicrohttpd
VERSION=0.4.6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) --disable-shared

LIBDYLIB=$(PLATFORM)/src/daemon/.libs/$(LIBNAME).a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../fix-android-include.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/src/daemon
	$(MAKE) -C $(PLATFORM)/src/include install

.installed-$(PLATFORM): $(LIBDYLIB)
	rm -f $(PREFIX)/lib/libmicrohttpd.so $(PREFIX)/lib/libmicrohttpd.so.5 $(PREFIX)/lib/libmicrohttpd.so.5.2.1
	$(MAKE) -C $(PLATFORM)/src/daemon install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

