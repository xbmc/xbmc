include ../../Makefile.include LIBBLURAY-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBBLURAY-VERSION Makefile ../../download-files.include \
                                  001-darwinembed_DiskArbitration-revert.patch \
                                  002-darwin-dlopen_searchpath.patch \
                                  003-win-libxml_searchname.patch \
                                  tvos.patch
MESON_BUILD_TYPE=release

ifeq ($(DEBUG_BUILD), yes)
  MESON_BUILD_TYPE=debug
endif

# configuration settings
CONFIGURE = $(NATIVEPREFIX)/bin/python3 $(NATIVEPREFIX)/bin/meson setup \
  --prefix=$(PREFIX) \
  --libdir=lib \
  --buildtype=$(MESON_BUILD_TYPE) \
  -Denable_tools=false \
  -Dembed_udfread=false \
  . build

ifeq ($(CROSS_COMPILING), yes)
  CONFIGURE += --cross-file=$(PREFIX)/share/cross-file.meson
  export CC=$(CC_FOR_BUILD)
  export CXX=$(CXX_FOR_BUILD)
  export CFLAGS=$(CFLAGS_FOR_BUILD)
  export CXXFLAGS=$(CXXFLAGS_FOR_BUILD)
else
  export CC CXX CFLAGS CXXFLAGS
endif
export PKG_CONFIG_LIBDIR=$(PREFIX)/share/pkgconfig

ifeq ($(OS), osx)
  # java and ant are dependencies for bdjs
  export ANT_HOME=$(PREFIX)/share/ant
  export PATH:=$(PREFIX)/share/ant/bin:$(PATH)
endif

LIBDYLIB=$(PLATFORM)/build/src/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../001-darwinembed_DiskArbitration-revert.patch
endif
	cd $(PLATFORM); patch -p1 -i ../002-darwin-dlopen_searchpath.patch
	cd $(PLATFORM); patch -p1 -i ../003-win-libxml_searchname.patch
ifeq ($(TARGET_PLATFORM),appletvos)
	cd $(PLATFORM); patch -p1 -i ../tvos.patch
endif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NATIVEPREFIX)/bin/ninja -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
