include ../../Makefile.include LIBASS-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile LIBASS-VERSION ../../download-files.include \
                                  01-win-CMakeLists.patch \
                                  02-win-dwrite-fontload.patch \
                                  03-win-dwrite-prevent-ass_set_fonts_dir.patch \
                                  04-win-nullcheck-shared_hdc.patch

# set ASS_BUILD_SYSTEM to cmake to use cmake build system patches
ASS_BUILD_SYSTEM=make

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
                      --disable-shared

LIBDYLIB=$(PLATFORM)/$(LIBNAME)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-win-CMakeLists.patch
	cd $(PLATFORM); patch -p1 -i ../02-win-dwrite-fontload.patch
	cd $(PLATFORM); patch -p1 -i ../03-win-dwrite-prevent-ass_set_fonts_dir.patch
	cd $(PLATFORM); patch -p1 -i ../04-win-nullcheck-shared_hdc.patch
ifeq ($(ASS_BUILD_SYSTEM), cmake)
	cd $(PLATFORM)/build; $(CMAKE) -DARCH=$(MESON_CPU) -DNATIVEPREFIX=$(NATIVEPREFIX) -DCMAKE_BUILD_TYPE=Release ..
else
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)
endif

$(LIBDYLIB): $(PLATFORM)
ifeq ($(ASS_BUILD_SYSTEM), cmake)
	$(MAKE) -C $(PLATFORM)/build
else
	$(MAKE) -C $(PLATFORM)
endif

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq ($(ASS_BUILD_SYSTEM), cmake)
	$(MAKE) -C $(PLATFORM)/build install
else
	$(MAKE) -C $(PLATFORM) install
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
