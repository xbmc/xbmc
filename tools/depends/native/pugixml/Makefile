include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=pugixml
VERSION=1.16
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=d9fe54d844b75ecced304045dc160aa3811c8567cceed06cda4b1382b07f52cfbf02b6933c2fbe964bd5bb4678b8eb863ea5a7b708b51fc6a57397d12b01d384
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/libpugixml.a

CMAKE_OPTIONS := -DCMAKE_INSTALL_LIBDIR=lib \
                 -DCMAKE_INSTALL_PREFIX=$(NATIVEPREFIX) \
                 $(CMAKE_OPTIONS)

BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(NATIVEPREFIX)/bin/cmake $(CMAKE_OPTIONS) ..
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
