include ../../Makefile.include
DEPS= ../../Makefile.include Makefile Imaging-1.1.7-access.patch \
      Imaging-1.1.7-crosscompiling-0.1.patch

VERSION.TXT := $(XBMCROOT)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=Imaging
VERSION=1.1.7
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS)"
ifeq ($(OS),android)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm"
endif
ifeq ($(OS),osx)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -lpython"
endif
ifeq ($(OS),ios)
PYTHON_O=$(abs_top_srcdir)/target/python27/$(PLATFORM)/Modules/python.o
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" CCSHARED="$(CC) $(CFLAGS) $(PYTHON_O)" LDFLAGS="$(LDFLAGS)"
endif

LIBDYLIB=$(PLATFORM)/dist/PIL-$(VERSION)-py2.7-$(OS)-$(CPU).egg

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-crosscompiling-0.1.patch
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-access.patch
	cd $(PLATFORM); patch -p0 < ../Imaging-1.1.7-zip-include.patch
	cd $(PLATFORM); patch -p0 < ../Imaging-1.1.7-remove-zlib-check.patch
	cd $(PLATFORM); patch -p1 < ../Imaging-1.1.7-bz2-link.patch
	cd $(PLATFORM); sed -ie "s|"darwin"|"darwinNot"|g" "setup.py"

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py build_ext --plat-name $(OS)-$(CPU)

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq ($(OS),android)
	mkdir -p $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil/lib
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py install --install-lib  $(PREFIX)/share/$(APP_NAME)/addons/script.module.pil/lib
else
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py install --prefix=$(PREFIX)
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
