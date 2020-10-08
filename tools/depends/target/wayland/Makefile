include ../../Makefile.include
DEPS=../../Makefile.include Makefile

# lib name, version
LIBNAME=wayland
VERSION=1.17.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --with-host-scanner --disable-documentation --disable-dtd-validation

LIBDYLIB=$(PLATFORM)/.libs/libwayland-client.la

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

	# remove the target wayland scanner from the sysroot. We only want to use the native one
	rm -f $(PREFIX)/bin/wayland-scanner
	rm -f $(PREFIX)/lib/pkgconfig/wayland-scanner.pc
	ln -sf $(NATIVEPREFIX)/lib/pkgconfig/wayland-scanner.pc $(PREFIX)/lib/pkgconfig/wayland-scanner.pc
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
