include ../../Makefile.include WAYLANDPP-SCANNER-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile WAYLANDPP-SCANNER-VERSION ../../download-files.include 001-fix-gcc13-build.patch

CMAKE_OPTIONS := -DBUILD_DOCUMENTATION=OFF \
                 -DBUILD_LIBRARIES=OFF \
                 -DBUILD_EXAMPLES=OFF \
                 -DBUILD_SCANNER=ON \
                 -DCMAKE_INSTALL_LIBDIR=lib \
                 -DCMAKE_INSTALL_PREFIX=$(NATIVEPREFIX) \
                 $(CMAKE_OPTIONS)

BUILDDIR = $(PLATFORM)/build

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-fix-gcc13-build.patch
	mkdir -p $(BUILDDIR)
	cd $(BUILDDIR); $(NATIVEPREFIX)/bin/cmake $(CMAKE_OPTIONS) ..

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
