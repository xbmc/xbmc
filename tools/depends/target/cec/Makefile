include ../../Makefile.include CEC-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CEC-VERSION ../../download-files.include \
                            001-all-cmakelists.patch \
                            002-all-libceccmakelists.patch \
                            003-all-remove_git_info.patch \
                            004-all-PR674.patch \
                            005-all-PR675-1.patch \
                            006-all-PR675-2.patch \
                            007-all-cmake-version.patch

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(BYPRODUCT)
ifeq (darwin, $(findstring darwin, $(HOST)))
	LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(BYPRODUCT_MAC)
endif

CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=1 \
              -DSKIP_PYTHON_WRAPPER:STRING=1 \
              -DCMAKE_PLATFORM_NO_VERSIONED_SONAME=1 \
              -DDISABLE_BUILDINFO=ON \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DCMAKE_BUILD_TYPE=Debug

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-all-cmakelists.patch
	cd $(PLATFORM); patch -p1 -i ../002-all-libceccmakelists.patch
	cd $(PLATFORM); patch -p1 -i ../003-all-remove_git_info.patch
	cd $(PLATFORM); patch -p1 -i ../004-all-PR674.patch
	cd $(PLATFORM); patch -p1 -i ../005-all-PR675-1.patch
	cd $(PLATFORM); patch -p1 -i ../006-all-PR675-2.patch
	cd $(PLATFORM); patch -p1 -i ../007-all-cmake-version.patch
	cd $(PLATFORM)/build; $(CMAKE) ${CMAKE_OPTIONS} ..

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

