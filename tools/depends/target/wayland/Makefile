include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=wayland
VERSION=1.18.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=e30199e30c2bbd361ee695b4f3f7a4e264f10ed8f46f2c90762b5739fc578ae757dc39aa0258d8fbf0ed418553470bccd4b2730ed9705481cfccdab5de96a8fc
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --with-host-scanner --disable-documentation --disable-dtd-validation

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
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
