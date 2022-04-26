include ../../Makefile.include
DEPS=../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libicundk
VERSION=master
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
BASE_URL=https://gitlab.com/koying/libicundk/-/archive/master
SHA256=48C3694692661260A6CD8B480E0B9CF1AE09240EE5E772A68870B1054D0D9BAE
include ../../download-files.include

BUILDDIR=$(PLATFORM)/build-cmake
LIBDYLIB=$(BUILDDIR)/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(CMAKE) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
