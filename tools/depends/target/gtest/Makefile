include ../../Makefile.include GTEST-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile GTEST-VERSION ../../download-files.include \
                        01-win-stl_algorithms_cxxflag.patch \
                        02-win-pdb_install_paths.patch

CMAKE_OPTIONS := -DBUILD_GMOCK:BOOL=OFF

ifeq ($(NATIVE_OS), osx)
  # build multiarch lib for osx
  CMAKE_OPTIONS+= -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
endif

LIBDYLIB=$(PLATFORM)/build/lib/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(TARGET_PLATFORM),windows)
	cd $(PLATFORM); patch -p1 -i ../01-win-stl_algorithms_cxxflag.patch
	cd $(PLATFORM); patch -p1 -i ../02-win-pdb_install_paths.patch
endif
	cd $(PLATFORM); $(CMAKE) -B build $(CMAKE_OPTIONS)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
