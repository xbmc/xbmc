include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-disable-openssl-plist.patch ../../download-files.include

# lib name, version
LIBNAME=ldid
VERSION=2.1.4-b8252d1
# tarball source: https://github.com/sbingner/ldid/commit/b8252d1f755ee95a589c18cdab8798c666325207
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=a54825e675d31902c9d75d5228a684e7ffced0bab9a3b213eb5a69f2274571d834af17dc6f75452be19ad6bae4e5ebd243678d48679df9e000c41aa58c151d0a
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/ldid

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-openssl-plist.patch
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM) LDID_VERSION=$(VERSION)

.installed-$(PLATFORM): $(LIBDYLIB)
	cp $(PLATFORM)/ldid $(NATIVEPREFIX)/bin
	touch $@

clean:
	rm -r $(PLATFORM)
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
