include ../Makefile.include
DEPS= ../Makefile.include 0001-dontbuildswig.patch 0002-missing-libs.patch \
      0003-no-so-version.patch Makefile

# lib name, version
LIBNAME=libplist
VERSION=1.6
SOURCE=$(LIBNAME)-$(VERSION)
FILENAME=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/build/src/libplist++.so

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../0001-dontbuildswig.patch
	cd $(PLATFORM); patch -p0 < ../0002-missing-libs.patch
	cd $(PLATFORM); patch -p0 < ../0003-no-so-version.patch
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; CC=$(CC) CXX=$(CXX) cmake        \
	  -DCMAKE_C_FLAGS:STRING="$(CPPFLAGS) $(CFLAGS)"     \
	  -DCMAKE_CXX_FLAGS:STRING="$(CPPFLAGS) $(CXXFLAGS)" \
	  -DCMAKE_INCLUDE_PATH=$(PREFIX)/include             \
	  -DCMAKE_LIBRARY_PATH=$(PREFIX)/lib                 \
	  -DCMAKE_INSTALL_NAME_DIR=$(PREFIX)/lib             \
	  -DCMAKE_INSTALL_PREFIX=$(PREFIX)                   \
	  -DCMAKE_SYSTEM_NAME=Linux                          \
	  -DCMAKE_FIND_ROOT_PATH=$(platform_sdk_path) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install

	rm -f $(PREFIX)/lib/libplist.la
	-$(READELF) --dynamic $(PREFIX)/lib/libplist.so | grep ibrary

	rm -f $(PREFIX)/lib/libplist++.la
	-$(READELF) --dynamic $(PREFIX)/lib/libplist++.so | grep ibrary
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

