include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-gcrypt-android-select.patch 02-fix-armv7-neon.patch 03-remove-cipher-gcm-armv8.patch 04-fix-o-flag-munging.patch

# lib name, version
LIBNAME=libgcrypt
VERSION=1.8.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

ifeq ($(OS),osx)
  CONFIGURE_FLAGS+= --disable-asm --disable-avx-support --disable-avx2-support
endif
ifeq ($(findstring arm64, $(CPU)), arm64)
  CONFIGURE_FLAGS+= --disable-asm
else
  CONFIGURE_FLAGS+= --disable-asm --disable-arm-crypto-support
endif


# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-doc --disable-shared \
	  $(CONFIGURE_FLAGS)

LIBDYLIB=$(PLATFORM)/src/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-gcrypt-android-select.patch
	cd $(PLATFORM); patch -p1 -i ../02-fix-armv7-neon.patch
ifneq ($(findstring arm64, $(CPU)), arm64)
	cd $(PLATFORM); patch -p1 -i ../03-remove-cipher-gcm-armv8.patch
endif
	cd $(PLATFORM); patch -p1 -i ../04-fix-o-flag-munging.patch
	# do not build the tests or docs
	sed -ie "s|doc tests||" "$(PLATFORM)/Makefile.am"
	sed -ie "s|\$$(doc) tests||" "$(PLATFORM)/Makefile.am"
	cd $(PLATFORM); $(AUTORECONF) -vif
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
