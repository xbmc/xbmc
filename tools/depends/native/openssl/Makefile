include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=openssl
VERSION=1.1.0h
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=MACHINE=$(PLATFORM) ./config no-shared zlib no-asm --prefix=$(NATIVEPREFIX) --with-zlib-include=$(NATIVEPREFIX)/include --with-zlib-lib=$(NATIVEPREFIX)/lib

ifeq ($(NATIVE_OS), osx)
  CONFIGURE=./Configure darwin64-x86_64-cc zlib no-asm no-shared --prefix=$(NATIVEPREFIX)
endif

CFLAGS="$(NATIVE_CFLAGS)"
LDFLAGS="$(NATIVE_LDFLAGS)"
CXXFLAGS="$(NATIVE_CXXFLAGS)"
CPPFLAGS="$(NATIVE_CPPFLAGS)"

export CFLAGS CPPFLAGS CXXFLAGS LDFLAGS

LIBDYLIB=$(PLATFORM)/libssl.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install_sw
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

