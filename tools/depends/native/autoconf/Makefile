include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=autoconf
VERSION=2.71
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/bin/autoconf

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
