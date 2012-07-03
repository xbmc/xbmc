include ../Makefile.include
DEPS= ../Makefile.include Makefile

LIBNAME=taglib
VERSION=1.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz

LIBDYLIB=$(PLATFORM)/build/$(LIBNAME)/libtag.so
all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; CC=$(CC) CXX=$(CXX) cmake        \
	  -DCMAKE_C_FLAGS:STRING="$(CPPFLAGS) $(CFLAGS)"     \
	  -DCMAKE_CXX_FLAGS:STRING="$(CPPFLAGS) $(CXXFLAGS)" \
	  -DCMAKE_INCLUDE_PATH=$(PREFIX)/include             \
	  -DCMAKE_LIBRARY_PATH=$(PREFIX)/lib                 \
	  -DCMAKE_INSTALL_NAME_DIR=$(PREFIX)/lib             \
	  -DCMAKE_INSTALL_PREFIX=$(PREFIX)                   \
	  -DCMAKE_FIND_ROOT_PATH="$(TOOLCHAIN)/sysroot/usr;$(PREFIX)" ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	touch $@
.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	rm -f $(PREFIX)/lib/libtag_c.* $(PREFIX)/lib/libtag.so $(PREFIX)/lib/libtag.so.1
	mv -f $(PREFIX)/lib/libtag.so.1.11.0 $(PREFIX)/lib/libxbtag.so
	ln -sf $(PREFIX)/lib/libxbtag.so $(PREFIX)/lib/libtag.so
	$(RPL) -e "libtag.so.1" "libxbtag.so" $(PREFIX)/lib/libxbtag.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbtag.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
