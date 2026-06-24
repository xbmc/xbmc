include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-disable_testsuite.patch ../../download-files.include

# lib name, version
LIBNAME=nettle
VERSION=4.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=833303d94f5a67094011ad4dd931fffdb9adf679b4df241a544a08194a66e6e449398b704ba5b29d52c1c3b5ada6f6dc18b24d1adb3382a68470a11645bf13cd
include ../../download-files.include

ifeq ($(OS),osx)
ifeq ($(CPU),arm64)
  CONFIGURE_FLAGS=-disable-assembler
endif
endif

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared --disable-fat --disable-openssl --disable-documentation $(CONFIGURE_FLAGS)

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable_testsuite.patch
	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE) CCPIC=" "
	touch $@

.installed-$(PLATFORM): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
