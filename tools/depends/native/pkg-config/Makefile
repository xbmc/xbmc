include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile

# lib name, version
# lib name, version
LIBNAME=pkg-config
VERSION=0.29.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PC_PATH=$(PREFIX)/lib/pkgconfig:$(PREFIX)/share/pkgconfig
ifeq ($(CROSS_COMPILING),no)
PC_PATH:=$(PC_PATH):/usr/lib/pkgconfig:/usr/lib/$(HOST)/pkgconfig:/usr/share/pkgconfig
endif

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-indirect-deps --with-pc-path=$(PC_PATH) --with-internal-glib
LIBDYLIB=$(PLATFORM)/pkg-config

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)
	cd $(PLATFORM); sed -ie "s|LN = ln|LN = ln -f|" Makefile

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
