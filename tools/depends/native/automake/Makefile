include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-fix-help2man-error.patch ../../download-files.include

# lib name, version
LIBNAME=automake
VERSION=1.16.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=3084ae543aa3fb5a05104ffb2e66cfa9a53080f2343c44809707fd648516869511500dba50dae67ff10f92a1bf3b5a92b2a0fa01cda30adb69b9da03994d9d88
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(NATIVEPREFIX)

LIBDYLIB=$(PLATFORM)/bin/automake

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-fix-help2man-error.patch
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
