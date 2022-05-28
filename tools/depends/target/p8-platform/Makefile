include ../../Makefile.include P8-PLATFORM-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile P8-PLATFORM-VERSION ../../download-files.include \
                              001-all-fix-c++17-support.patch \
                              002-all-fixcmakeinstall.patch \
                              003-all-cmake_tweakversion.patch

LIBDYLIB=$(PLATFORM)/build/$(BYPRODUCT)

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../001-all-fix-c++17-support.patch
	cd $(PLATFORM); patch -p1 -i ../002-all-fixcmakeinstall.patch
	cd $(PLATFORM); patch -p1 -i ../003-all-cmake_tweakversion.patch
	cd $(PLATFORM)/build; $(CMAKE) -DBUILD_SHARED_LIBS=0 ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
