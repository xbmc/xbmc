include ../../Makefile.include
DEPS= ../../Makefile.include 0001-dontbuildswig.patch Makefile

# lib name, version
LIBNAME=libplist
VERSION=1.6
SOURCE=$(LIBNAME)-$(VERSION)
FILENAME=v$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
ifeq (darwin, $(findstring darwin, $(HOST)))
LIBDYLIB=$(PLATFORM)/build/src/libplist++.dylib
else
LIBDYLIB=$(PLATFORM)/build/src/libplist++.so
endif
CFLAGS+=-fvisibility=default
all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../0001-dontbuildswig.patch
	cd $(PLATFORM); patch -p1 < ../0002-fixclang.diff
	cd $(PLATFORM); patch -p1 < ../0003-fixc++1.patch
	cd $(PLATFORM); sed -ie 's/TARGET_LINK_LIBRARIES( plist \(.*\))/TARGET_LINK_LIBRARIES( plist \1 z m)  /' src/CMakeLists.txt
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM)/build; $(CMAKE) VERBOSE=1 -DCMAKE_C_FLAGS="$(CFLAGS)" -DCMAKE_LD_FLAGS="$(LDFLAGS)" ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)/build VERBOSE=1

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

