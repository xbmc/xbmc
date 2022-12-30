include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=autoconf
VERSION=2.71
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=73d32b4adcbe24e3bafa9f43f59ed3b6efbd3de0f194e5ec90375f35da1199c583f5d3e89139b7edbad35171403709270e339ffa56a2ecb9b3123e9285021ff0
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/bin/autoconf

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(CONFIGURE)
	$(MAKE) -C $(PLATFORM) install
# patch autoreconf to not use gtkdocize. Details: https://savannah.gnu.org/support/?110503
	cd $(NATIVEPREFIX); sed -ie 's|$uses_gtkdoc = 1              if \$$macro eq "GTK_DOC_CHECK";|$uses_gtkdoc = 0;  #            if \$$macro eq "GTK_DOC_CHECK";|' bin/autoreconf

.installed-$(PLATFORM): $(LIBDYLIB)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
