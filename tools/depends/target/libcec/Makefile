include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libcec
VERSION_MAJOR=3
VERSION_MINOR=0
VERSION_PATCH=0

VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
SOURCE=$(LIBNAME)-$(VERSION)-6
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME).la

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) -DBUILD_SHARED_LIBS=1 -DSKIP_PYTHON_WRAPPER:STRING=1 -DCMAKE_INSTALL_LIBDIR=$(PREFIX)/lib ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/$(LIBNAME).la $(PREFIX)/lib/$(LIBNAME).so $(PREFIX)/lib/$(LIBNAME).so.$(VERSION_MAJOR) $(PREFIX)/lib/$(LIBNAME).so.$(VERSION_MAJOR).$(VERSION_MINOR)
	mv -f $(PREFIX)/lib/$(LIBNAME).so.$(VERSION) $(PREFIX)/lib/$(LIBNAME).so
	$(RPL) -e "$(LIBNAME).so.$(VERSION_MAJOR)" "$(LIBNAME).so\x00\x00" $(PREFIX)/lib/$(LIBNAME).so
	-$(READELF) --dynamic $(PREFIX)/lib/$(LIBNAME).so | grep ibrary
endif
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

