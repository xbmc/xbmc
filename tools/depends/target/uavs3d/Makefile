include ../../Makefile.include UAVS3D-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile UAVS3D-VERSION ../../download-files.include \
                      01-all-enable_prog.patch \
                      02-all-arm_removeflags.patch \
                      03-all-version.h.patch \
                      04-all-pthread.patch

LIBDYLIB=$(PLATFORM)/build/source/$(BYPRODUCT)
all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../01-all-enable_prog.patch
	cd $(PLATFORM); patch -p1 -i ../02-all-arm_removeflags.patch
	cd $(PLATFORM); patch -p1 -i ../03-all-version.h.patch
	cd $(PLATFORM); patch -p1 -i ../04-all-pthread.patch
	cd $(PLATFORM)/build; $(CMAKE) -DCOMPILE_10BIT=ON -DCMAKE_BUILD_TYPE=Debug ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
