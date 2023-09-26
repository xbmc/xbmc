include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include 001-fix-gcc13-build.patch

# lib name, version
LIBNAME=waylandpp
VERSION=0.2.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=bf1b8a9e69b87547fc65989b9eaff88a442d8b2f01f5446cef960000b093390b1e557536837fbf38bb6d9a4f93e3985ea34c3253f94925b0f571b4606c980832
include ../../download-files.include

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
