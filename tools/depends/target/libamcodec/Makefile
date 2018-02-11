include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-flags.patch 02-no-amadec.patch

# lib name, version
LIBNAME=libamcodec
VERSION=20170630
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared

LIBDYLIB=$(PLATFORM)/amcodec/libamcodec.so.0.0

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-flags.patch
	cd $(PLATFORM); patch -p1 -i ../02-no-amadec.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/amcodec CC="$(CC)" EXT_CFLAGS="$(CFLAGS)" LD="$(LD)" EXT_LDFLAGS="$(LDFLAGS)"

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/amcodec install CC="$(CC)" LD="$(LD)" PREFIX="$(PREFIX)" HEADERS_DIR="$(PREFIX)/include/amcodec"
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
