include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-android.patch

LIBNAME=mariadb
VERSION=3.0.3
ARCHIVE=$(LIBNAME)-connector-c-$(VERSION)-src.tar.gz

LIBDYLIB=$(PLATFORM)/build/lib$(LIBNAME)/lib$(LIBNAME)client.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-android.patch
	sed -ie 's| "DYNAMIC" | "STATIC" |' $(PLATFORM)/cmake/plugins.cmake
	cd $(PLATFORM)/build; $(CMAKE) -DAUTH_GSSAPI=OFF -DWITH_UNITTEST:BOOL=OFF -DWITH_EXTERNAL_ZLIB:BOOL=ON -DWITH_CURL:BOOL=OFF ..

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
