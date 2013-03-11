include ../../Makefile.include
DEPS= ../../Makefile.include Makefile nasm.patch

# lib name, version
LIBNAME=flac
VERSION=1.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
export NASM=
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --disable-xmms-plugin --disable-cpplibs --disable-asm-optimizations

LIBDYLIB=$(PLATFORM)/src/libFLAC/.libs/libFLAC.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../autoreconf.patch
	cd $(PLATFORM); patch -p0 < ../nasm.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libFLAC.la $(PREFIX)/lib/libFLAC.so $(PREFIX)/lib/libFLAC.so.8
	mv -f $(PREFIX)/lib/libFLAC.so.8.2.0 $(PREFIX)/lib/libxbFLAC.so
	ln -sf $(PREFIX)/lib/libxbFLAC.so $(PREFIX)/lib/libFLAC.so
	$(RPL) -e "libFLAC.so.8" "libxbFLAC.so" $(PREFIX)/lib/libxbFLAC.so
	-$(READELF) --dynamic $(PREFIX)/lib/libxbFLAC.so | grep ibrary
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

