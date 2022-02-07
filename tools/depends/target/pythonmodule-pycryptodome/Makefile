include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-nosetuptool.patch 02-android-dlopen.patch 03-obey-crosscompileflags.patch ../../download-files.include

# lib name, version
LIBNAME=pycryptodome
VERSION=3.4.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=066ac92f8c4fc3aa4d2ba955d9e7665938c96b60ab516783a9446e718e2e9467f2411710d5ab203c8ae66945b1c0c3de2149d82d3f6f04e8517a57f82e20a879
include ../../download-files.include

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PYTHON_SITE_PKG)"
ifeq ($(OS),android)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm" PYTHONPATH="$(PYTHON_SITE_PKG)"
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
ifeq ($(OS),darwin_embedded)
# Work around an issue with xcode 11 stripping -arch arm64 flags.
# Not required for xcode 12+, but doesnt hurt either.
CFLAGS+= -target arm64-apple-darwin
endif
#ensure that only our target ldflags are passed to the python build
LDSHARED:=$(CC) -bundle -undefined dynamic_lookup $(LDFLAGS)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" CCSHARED="$(CC) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PYTHON_SITE_PKG)" LDSHARED="$(LDSHARED)"
endif

LIBDYLIB=$(PLATFORM)/build/lib.$(OS)-$(CPU)-$(PYTHON_VERSION)/Cryptodome
ifeq ($(NATIVE_OS), osx)
	LIBDYLIB=$(PLATFORM)/build/lib.macosx-10.4-x86_64-$(PYTHON_VERSION)/Cryptodome
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
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); touch .separate_namespace && $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py build_ext --plat-name $(OS)-$(TARGET_ARCH)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
