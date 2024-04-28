include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=gmp
VERSION=6.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=c99be0950a1d05a0297d65641dd35b75b74466f7bf03c9e8a99895a3b2f9a0856cd17887738fa51cf7499781b65c049769271cbcb77d057d2e9f1ec52e07dd84
include ../../download-files.include

# ABI selection
ifeq ($(OS),linux)
  ifeq (i686,$(findstring i686,$(HOST)))
    ABI=ABI=32
  else
  ifeq (i386,$(findstring i386,$(HOST)))
    ABI=ABI=32
  endif
  endif
endif

ifeq ($(OS),darwin_embedded)
CONFIGURE_FLAGS=--disable-assembly
endif
ifeq ($(OS),osx)
CONFIGURE_FLAGS=--with-pic
ifeq ($(CPU),arm64)
CONFIGURE_FLAGS+= --disable-assembly
endif
endif
ifeq ($(OS), android)
CONFIGURE_FLAGS=--with-pic
endif
ifeq ($(NATIVE_OS), osx)
CONFIGURE_FLAGS+= CC_FOR_BUILD="$(CC_FOR_BUILD) $(NATIVE_CFLAGS)"
endif

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-shared $(ABI) $(CONFIGURE_FLAGS)


LIBDYLIB=$(PLATFORM)/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM)/config.status: $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)/config.status
	@echo "DYLIB: $(LIBDYLIB)"
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
