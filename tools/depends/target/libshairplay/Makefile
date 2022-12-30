include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 0001-configure-fix-dns-sd-check.patch ../../download-files.include

# lib name, version
LIBNAME=shairplay
VERSION=096b61a
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=592a001954fd3ca86a81a907fef3892ebb53f4b91e937a049ec048e3473919f1c6e00fdf4a755d187c0b07afb0062e3613587f95817ec0786eeac5c18848ae1f
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)


LIBDYLIB=$(PLATFORM)/src/lib/.libs/libshairplay.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../0001-configure-fix-dns-sd-check.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

