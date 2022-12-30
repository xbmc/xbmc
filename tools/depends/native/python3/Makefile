include ../../Makefile.include PYTHON3-VERSION ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile PYTHON3-VERSION ../../download-files.include \
                                01-distutil-flags.patch

CONFIGURE=./configure --prefix=$(NATIVEPREFIX) \
                      --disable-shared \
                      --disable-framework \
                      --without-pymalloc \
                      --with-system-ffi

NATIVE_SITEPACKAGES=$(NATIVEPREFIX)/lib/python$(PYTHON_VERSION)/site-packages

ifeq ($(OS),linux)
  CONFIGURE += --with-system-expat
endif

LIBDYLIB=$(PLATFORM)/python
ifeq ($(NATIVE_OS), osx)
    # Case insensitive FS will add .exe to buildexe regardless of --with-suffix=no
	LIBDYLIB=$(PLATFORM)/python.exe
endif

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); patch -p1 -i ../01-distutil-flags.patch
	cd $(PLATFORM); $(MAKE) install
# Sed patch setuptools that is installed via ensurepip as we cant patch the source
	cd $(NATIVE_SITEPACKAGES); sed -ie "s|cflags = cflags + ' ' + os.environ\['CFLAGS'\]|cflags = os.environ\['CFLAGS'\]|" setuptools/_distutils/sysconfig.py
	touch $(LIBDYLIB)
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
