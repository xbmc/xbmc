include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-android.patch 04-pthread.patch ../../download-files.include

LIBNAME=mariadb
VERSION=3.1.15
ARCHIVE=$(LIBNAME)-connector-c-$(VERSION).tar.gz
SHA512=c64a69fecf1e0053748fb24b82614fc8d862de79d139f848283a504d72b521bc6bf29119a86fc9cca57c3578812702483cc408c8a684e35887108f172eaa3c56
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/lib$(LIBNAME)/lib$(LIBNAME)client.a

# build all plugins as static
PLUGIN_BUILD_FLAGS=-DCLIENT_PLUGIN_DIALOG=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_SHA256_PASSWORD=STATIC -DCLIENT_PLUGIN_CACHING_SHA2_PASSWORD=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_MYSQL_CLEAR_PASSWORD=STATIC -DCLIENT_PLUGIN_MYSQL_OLD_PASSWORD=STATIC
PLUGIN_BUILD_FLAGS+=-DCLIENT_PLUGIN_CLIENT_ED25519=STATIC -DCLIENT_PLUGIN_AUTH_GSSAPI_CLIENT=STATIC

ifeq ($(TARGET_PLATFORM),appletvos)
  PLUGIN_BUILD_FLAGS+=-DHAVE_UCONTEXT_H= -DHAVE_FILE_UCONTEXT_H=
endif

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-android.patch
ifeq ($(OS),linux)
	cd $(PLATFORM); patch -p1 -i ../04-pthread.patch
endif
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
