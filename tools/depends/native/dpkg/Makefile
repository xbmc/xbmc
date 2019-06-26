include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile 01-no-gnu-patch.patch 02-perl-min-version.patch 03-lzma-compression.patch

# lib name, version
LIBNAME=dpkg
VERSION=1.19.7
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE).tar.xz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-static --disable-shared --disable-devel-docs \
	  --disable-linker-optimizations --disable-nls --disable-dselect --disable-start-stop-daemon \
	  ac_cv_header_libintl_h=no

LIBDYLIB=$(PLATFORM)/dpkg-deb/dpkg-deb

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-no-gnu-patch.patch
	cd $(PLATFORM); patch -p1 -i ../02-perl-min-version.patch
	cd $(PLATFORM); patch -p1 -i ../03-lzma-compression.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/dpkg-deb install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
