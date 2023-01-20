include ../../Makefile.include.in
DEPS=Makefile
PREFIX=$(CURDIR)/../../pre-build-deps
PLATFORM=native
TARBALLS_LOCATION=$(PREFIX)
RETRIEVE_TOOL=curl
ARCHIVE_TOOL=tar
# lib name, version
LIBNAME=m4
VERSION=1.4.19
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/src/$(LIBNAME)

all: $(PREFIX)/bin/$(LIBNAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	mkdir -p $(TARBALLS_LOCATION)
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(ARCHIVE) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

$(PREFIX)/bin/$(LIBNAME): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM)
