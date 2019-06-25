include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-static-lib.patch
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

# lib name, version
LIBNAME=giflib
VERSION=5.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/libgif.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-static-lib.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) libgif.a CFLAGS="$(NATIVE_CFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install-include PREFIX="$(PREFIX)"
	$(MAKE) -C $(PLATFORM) install-lib PREFIX="$(PREFIX)"
	touch $@
clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)
	rm -rf $(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
