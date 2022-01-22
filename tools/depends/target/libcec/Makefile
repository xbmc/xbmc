include ../../Makefile.include
DEPS = ../../Makefile.include Makefile remove_git_info.patch ../../download-files.include

# lib name, version
LIBNAME=libcec
VERSION_MAJOR=4
VERSION_MINOR=0
VERSION_PATCH=7

VERSION=$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=424540a45f9cae3d5dcccc615d487c45033f9cdeb665b8176832495597e2cd58ef7681e13b52f6a32f8a40e1146c04a1a383f5785ea1e731c5c517a9a7843a81
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(LIBNAME).so
ifeq (darwin, $(findstring darwin, $(HOST)))
	LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(LIBNAME).dylib
endif

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
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

