include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-android.patch 02-symbol_rename-gnutls_clash.patch 03-cmake3.20_syntax_fix.patch

LIBNAME=mariadb
VERSION=3.1.12
ARCHIVE=$(LIBNAME)-connector-c-$(VERSION)-src.tar.gz

BASE_URL=https://downloads.mariadb.org/interstitial/connector-c-3.1.12

LIBDYLIB=$(PLATFORM)/build/lib$(LIBNAME)/lib$(LIBNAME)client.a

# build all plugins as static
PLUGIN_BUILD_FLAGS=-DCLIENT_PLUGIN_DIALOG=STATIC -DAUTH_GSSAPI_PLUGIN_TYPE=OFF
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD=STATIC -DCLIENT_PLUGIN_MYSQL_OLD_PASSWORD=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_mysql_native_password=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_CLIENT_ED25519=STATIC

ifeq ($(OS), linux)
  # OpenSSL now uses pthread_atfork(), and this requires -lpthread on Linux to compile
  PLUGIN_BUILD_FLAGS+=-DCMAKE_SHARED_LINKER_FLAGS="$(LDFLAGS) -pthread"
endif

ifeq ($(TARGET_PLATFORM),appletvos)
  PLUGIN_BUILD_FLAGS+=-DHAVE_UCONTEXT_H= -DHAVE_FILE_UCONTEXT_H=
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-android.patch
	cd $(PLATFORM); patch -p1 -i ../02-symbol_rename-gnutls_clash.patch
	cd $(PLATFORM); patch -p1 -i ../03-cmake3.20_syntax_fix.patch
	cd $(PLATFORM)/build; $(CMAKE) $(PLUGIN_BUILD_FLAGS) -DWITH_SSL=OPENSSL -DWITH_UNIT_TESTS:BOOL=OFF -DWITH_EXTERNAL_ZLIB:BOOL=ON -DWITH_CURL:BOOL=OFF ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	rm -f $(PREFIX)/lib/mariadb/*.so $(PREFIX)/lib/mariadb/*.dylib
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
