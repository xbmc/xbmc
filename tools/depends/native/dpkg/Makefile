include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile 01-no-gnu-patch.patch 02-perl-min-version.patch 03-lzma-compression.patch ../../download-files.include

# lib name, version
LIBNAME=dpkg
VERSION=1.19.7
SOURCE=$(LIBNAME)_$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=9ca441dc265baf5359c71617aef1c57504a7097c26ea57108b88dadc511bfa9918fcc765d8c67cec9def9916c5df92d6cabe508446dbc7223a29b45260445e81
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --enable-static --disable-shared --disable-devel-docs \
	  --disable-linker-optimizations --disable-nls --disable-dselect --disable-start-stop-daemon \
	  ac_cv_header_libintl_h=no

LIBDYLIB=$(PLATFORM)/dpkg-deb/dpkg-deb

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
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
