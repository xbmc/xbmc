include ../../Makefile.include
DEPS= ../../Makefile.include Makefile remove_git_info.patch

# lib name, version
LIBNAME=libcec
VERSION_MAJOR=4
VERSION_MINOR=0
VERSION_PATCH=7

VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(LIBNAME).so
ifeq (darwin, $(findstring darwin, $(HOST)))
	LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(LIBNAME).dylib
endif

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../remove_git_info.patch
	cd $(PLATFORM)/build; $(CMAKE) -DBUILD_SHARED_LIBS=1 -DSKIP_PYTHON_WRAPPER:STRING=1 -DCMAKE_INSTALL_LIBDIR=$(PREFIX)/lib -DCMAKE_PLATFORM_NO_VERSIONED_SONAME=1 ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
ifeq (darwin, $(findstring darwin, $(HOST)))
	install_name_tool -id $(PREFIX)/lib/$(LIBNAME).dylib $(PREFIX)/lib/$(LIBNAME).dylib
endif
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

