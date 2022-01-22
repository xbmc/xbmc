include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 001-darwinembed_DiskArbitration-revert.patch tvos.patch ../../download-files.include

# lib name, version
LIBNAME=libbluray
VERSION=1.3.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=3d5145e6fd7de099c07f937282112c7abb12a5590b7c0b965b00bddee3837ddfd1a30076aaa6d6278d07a5beee3856f602125983ae075ab30eceb6ac1bd9bcdc
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --exec-prefix=$(PREFIX) \
  --disable-examples --disable-doxygen-doc \
  --disable-bdjava-jar

LIBDYLIB=$(PLATFORM)/.libs/libbluray.la

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); ./bootstrap
	cd $(PLATFORM); $(CONFIGURE)
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../001-darwinembed_DiskArbitration-revert.patch
endif
ifeq ($(TARGET_PLATFORM),appletvos)
	cd $(PLATFORM); patch -p1 -i ../tvos.patch
endif

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
