include ../../Makefile.include PYTHONMODULE-PYCRYPTODOME-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile PYTHONMODULE-PYCRYPTODOME-VERSION ../../download-files.include \
                             01-revert-ctype.pythonapi-use.patch \
                             02-android-dlopen.patch

LDSHARED=$(CC) -shared

ifeq ($(OS),android)
  LDFLAGS+= -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
  ifeq ($(CPU),arm64)
    # Work around an issue with xcode 11 stripping -arch arm64 flags.
    # Not required for xcode 12+, but doesnt hurt either.
    CFLAGS+= -target arm64-apple-darwin
    LDFLAGS+= -target arm64-apple-darwin
  endif
  ifeq ($(OS), darwin_embedded)
    # AppStore Connect doesn't allow files of type bundle
    OUTPUT_FILE_TYPE_ARG=-dynamiclib
  else
    OUTPUT_FILE_TYPE_ARG=-bundle
  endif
  LDSHARED:=$(CC) $(OUTPUT_FILE_TYPE_ARG) -undefined dynamic_lookup $(LDFLAGS)
endif


export CC CFLAGS AR RANLIB
export LDSHARED LDFLAGS
export PYTHONPATH=$(PYTHON_SITE_PKG)
export SETUPTOOLS_EXT_SUFFIX=.so

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-revert-ctype.pythonapi-use.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../02-android-dlopen.patch
endif
	touch $@

.installed-$(PLATFORM): .configured-$(PLATFORM)
	cd $(PLATFORM); touch .separate_namespace && $(NATIVEPREFIX)/bin/python3 setup.py build_ext --plat-name $(OS)-$(CPU)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
