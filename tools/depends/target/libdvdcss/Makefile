include ../../Makefile.include LIBDVDCSS-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBDVDCSS-VERSION Makefile ../../download-files.include \
                                  01-all-libc_read_improvement.patch \
                                  02-darwinembedded-enablebuild.patch \
                                  03-win-uwpfixes.patch \
                                  04-win-open_wrapforce.patch

MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson setup \
  --prefix=$(PREFIX) \
  --libdir=lib \
  --buildtype=$(MESON_BUILD_TYPE) \
  -Denable_docs=false \
  -Denable_examples=false \
  . build

ifeq ($(TARGET_PLATFORM),webos)
  CONFIGURE += -DPATH_MAX=2048
endif

ifeq ($(CROSS_COMPILING), yes)
  CONFIGURE += --cross-file=$(PREFIX)/share/cross-file.meson
  export CC=$(CC_FOR_BUILD)
  export CXX=$(CXX_FOR_BUILD)
  export CFLAGS=$(CFLAGS_FOR_BUILD)
  export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
  export CC CXX CFLAGS CXXFLAGS
endif

LIBDYLIB=$(PLATFORM)/build/src/$(BYPRODUCT)

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-libc_read_improvement.patch
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../02-darwinembedded-enablebuild.patch
endif
ifeq ($(TARGET_PLATFORM),windows)
	cd $(PLATFORM); patch -p1 -i ../03-win-uwpfixes.patch
	cd $(PLATFORM); patch -p1 -i ../04-win-open_wrapforce.patch
endif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
