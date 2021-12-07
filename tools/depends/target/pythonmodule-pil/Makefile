include ../../Makefile.include
DEPS= ../../Makefile.include Makefile pillow-crosscompile.patch

# lib name, version
LIBNAME=Pillow
VERSION=8.4.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PYTHONPATH=$(PYTHON_SITE_PKG)
PILPATH=$(PYTHONPATH)
LDSHARED=$(CC) -shared

# Clear pkg-config data, as it will pull from Native pkg-config incorrectly
ifeq ($(CROSS_COMPILING), yes)
export PKG_CONFIG_PATH=
export PKG_CONFIG_LIBDIR=${PREFIX}/lib/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=${SDKROOT}
endif

ifeq ($(OS),android)
PILPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
PILPATHLIB=$(PILPATH)/lib
PYTHONPATH=$(PILPATH):$(PYTHON_SITE_PKG)
EXTRALDFLAGS=-L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm
else ifeq ($(OS),darwin_embedded)
PILPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
PILPATHLIB=$(PILPATH)/lib
PYTHONPATH=$(PILPATH):$(PYTHON_SITE_PKG)

# Work around an issue with xcode 11 stripping -arch arm64 flags.
# Not required for xcode 12+, but doesnt hurt either.
CFLAGS+= -target arm64-apple-darwin
endif

SED_FLAG=-i
ifeq (darwin, $(findstring darwin, $(BUILD)))
  SED_FLAG=-i ''
endif

ifeq (darwin, $(findstring darwin, $(HOST)))
#ensure that only our target ldflags are passed to the python build
LDSHARED=$(CC) -bundle -undefined dynamic_lookup
ZLIB_ROOT=ZLIB_ROOT="$(SDKROOT)/usr"
endif

BUILD_OPTS=--plat-name $(OS)-$(CPU) --disable-jpeg2000 --disable-webp --disable-imagequant --disable-tiff --disable-webp --disable-webpmux --disable-xcb --disable-lcms --disable-platform-guessing
CROSSFLAGS=$(ZLIB_ROOT) PYTHONXINCLUDE="$(PREFIX)/include/python$(PYTHON_VERSION)" PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(LDSHARED)" LDFLAGS="$(LDFLAGS) $(EXTRALDFLAGS)" PYTHONPATH="$(PYTHONPATH)"

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../pillow-crosscompile.patch

$(PILPATHLIB):
ifeq ($(OS),android)
	mkdir -p $(PILPATHLIB)
else ifeq ($(OS),darwin_embedded)
	mkdir -p $(PILPATHLIB)
endif

.installed-$(PLATFORM): $(PLATFORM) $(PILPATHLIB)
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py build_ext $(BUILD_OPTS) install --install-lib $(PILPATH)
ifeq ($(OS),android)
	cd $(PILPATHLIB) && unzip -o ../Pillow-*.egg
	cd $(PILPATHLIB)/PIL && \
          sed $(SED_FLAG) -e 's/import sys/import os, sys /' \
                          -e '/__file__/ s/_imaging/lib_imaging/g' \
                          -e 's/pkg_resources.resource_filename(__name__,/os.path.join(os.environ["KODI_ANDROID_LIBS"], /' \
                              _imaging*.py
	cd $(PILPATH) && rm -rf Pillow-*.egg
else ifeq ($(OS),darwin_embedded)
	cd $(PILPATHLIB) && unzip -o ../Pillow-*.egg
	cd $(PILPATH) && rm -rf Pillow-*.egg
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
