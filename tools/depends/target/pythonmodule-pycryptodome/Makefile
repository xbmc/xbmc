include ../../Makefile.include PYTHONMODULE-PYCRYPTODOME-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile PYTHONMODULE-PYCRYPTODOME-VERSION ../../download-files.include \
                             01-nosetuptool.patch 02-android-dlopen.patch \
                             03-obey-crosscompileflags.patch

LDSHARED=$(CC) -shared

ifeq ($(OS),android)
  LDFLAGS+= -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
  ifeq ($(CPU),arm64)
    # Work around an issue with xcode 11 stripping -arch arm64 flags.
    # Not required for xcode 12+, but doesnt hurt either.
    CFLAGS+= -target arm64-apple-darwin
  endif
  LDSHARED:=$(CC) -bundle -undefined dynamic_lookup
endif

ifeq ($(TARGET_PLATFORM),appletvos)
  # We cant export CFLAGS directly for tvos
  # CFLAGS are added to linker command, having -bundle (LDSHARED) and -fembed_bitcode (CFLAGS)
  # causes a failure: ld: -bundle and -bitcode_bundle (Xcode setting ENABLE_BITCODE=YES) cannot be used together
  export CC:=$(CC) $(CFLAGS)
else
  export CC CFLAGS
endif

export LDSHARED LDFLAGS
export PYTHONPATH=$(PYTHON_SITE_PKG)

LIBDYLIB=$(PLATFORM)/build/lib.$(OS)-$(CPU)-$(PYTHON_VERSION)/Cryptodome
ifeq ($(NATIVE_OS), osx)
  # this module will always recompile. currently the hardcoded 10.4 is incorrect
  # and would need to be generated based on sdk version that we dont currently collect
	LIBDYLIB=$(PLATFORM)/build/lib.macosx-10.4-$(CPU)-$(PYTHON_VERSION)/Cryptodome
endif

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-nosetuptool.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../02-android-dlopen.patch
endif
	cd $(PLATFORM); patch -p1 -i ../03-obey-crosscompileflags.patch

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); touch .separate_namespace && $(NATIVEPREFIX)/bin/python3 setup.py build_ext --plat-name $(OS)-$(CPU)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
