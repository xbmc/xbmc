include ../../Makefile.include.in
PREFIX=$(CURDIR)/../../pre-build-deps
PLATFORM=native
TARBALLS_LOCATION=$(PREFIX)
RETRIEVE_TOOL=curl
ARCHIVE_TOOL=tar
# lib name, version
LIBNAME=autoconf
VERSION=2.71
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

export PATH:=$(CURDIR)/../../pre-build-deps/bin:$(PATH)

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/bin/autoconf

all: $(PREFIX)/bin/$(LIBNAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	mkdir -p $(TARBALLS_LOCATION)
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(ARCHIVE) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(CONFIGURE)
	$(MAKE) -C $(PLATFORM)

$(PREFIX)/bin/$(LIBNAME): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
