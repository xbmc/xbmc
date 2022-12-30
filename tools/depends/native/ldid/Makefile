include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-disable-openssl-plist.patch ../../download-files.include

# lib name, version
LIBNAME=ldid
VERSION=2.1.2-b462a92
# tarball source: https://github.com/sbingner/ldid/commit/b462a92349c57f38f2a6844adabee940afe01ca6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=9582d747f55e8fd1d54c20b6a3f0825d2ab0cfaffeb6992e53b39343b3aa3936f31e6353d59518f294ecc154adf687fd2dee0405d9b2e594cc16b19409f3c0f4
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/ldid

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-openssl-plist.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	cp $(PLATFORM)/ldid $(NATIVEPREFIX)/bin
	touch $@

clean:
	rm -r $(PLATFORM)
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
