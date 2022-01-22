include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=freetype
VERSION=2.11.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=0848678482fbe20603a866f02da82c91122014d6f815ba4f1d9c03601c32e3ceb781f721c2b4427b6117d7c9742018af8dbb26566faf018595c70b50f8db3f08
include ../../download-files.include

# Freetype has a circular dependency with harfbuzz.
# To enable harfbuzz support in freetype, we build this first to bootstrap harfbuzz
# and then build freetype2 again with harfbuzz support

# configuration settings
# force using internal libtool
export LIBTOOL=builds/unix/libtool
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) builds/unix; \
          ./configure --prefix=$(PREFIX) --disable-shared --with-png=no --with-harfbuzz=no

LIBDYLIB=$(PLATFORM)/objs/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
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
