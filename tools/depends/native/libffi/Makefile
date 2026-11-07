include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libffi
VERSION=3.8.0
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=a259d50f40b5dcde9cb6227bd105761e667290f107553b204a5a6cfdfbf2e6b022ed04be2ea4596da42961d11396125425def3cb67e2844a9eba90b6750c8c39
include ../../download-files.include

# configuration settings
CONFIGURE= CFLAGS="-fPIC" ./configure --prefix=$(NATIVEPREFIX) --disable-shared --disable-builddir


LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
