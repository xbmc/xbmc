include ../../Makefile.include
DEPS = ../../Makefile.include Makefile no-Werror.patch ../../download-files.include

# lib name, version
LIBNAME=libusb
VERSION=0.1.12
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=08c10e899f93b5f8567d1a1e1ed87dd5631ad0a1463975404709c1d165e545e340afb474a66788efcfdd8193542e2a2eb9cf133f55cbbf76441244191d0bb825
include ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --disable-build-docs

LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

all: $(LIBDYLIB) .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../no-Werror.patch
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
