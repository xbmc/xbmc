include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=wayland
VERSION=1.17.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=c5051aab5ff078b368c196ecfedb33ccd961265bb914845d7ed81de361bb86ae18299575baa6c4eceb0d82cf8b495e8293f31b51d1cbc05d84af0a199ab3f946
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --with-host-scanner --disable-documentation --disable-dtd-validation

LIBDYLIB=$(PLATFORM)/.libs/libwayland-client.la

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
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
