include ../../Makefile.include CEC-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile CEC-VERSION ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(BYPRODUCT)
ifeq (darwin, $(findstring darwin, $(HOST)))
	LIBDYLIB=$(PLATFORM)/build/src/$(LIBNAME)/$(BYPRODUCT_MAC)
endif

CMAKE_OPTIONS=-DBUILD_SHARED_LIBS=1 \
              -DSKIP_PYTHON_WRAPPER:STRING=1 \
              -DCMAKE_PLATFORM_NO_VERSIONED_SONAME=1 \
              -DDISABLE_BUILDINFO=ON \
              -DDISABLE_CLIENT=ON \
              -DDISABLE_STATIC=ON \
              -DCMAKE_INSTALL_LIBDIR=lib \
              -DCMAKE_INSTALL_INCLUDEDIR=include \
              -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
              -DCMAKE_BUILD_TYPE=Debug

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) ${CMAKE_OPTIONS} ..
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
ifeq (darwin, $(findstring darwin, $(HOST)))
	install_name_tool -id $(PREFIX)/lib/$(LIBNAME).dylib $(PREFIX)/lib/$(LIBNAME).dylib
endif
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)

