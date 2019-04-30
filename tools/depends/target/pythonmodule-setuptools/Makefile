include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=setuptools
VERSION=18.4
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python2.7/site-packages/"
ifeq ($(OS),android)
#PYTHONPATH=$(PREFIX)/share/$(APP_NAME)/addons/script.module.pil
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -L$(PREFIX)/lib/dummy-lib$(APP_NAME)/ -l$(APP_NAME) -lm" PYTHONPATH="$(PREFIX)/lib/python2.7/site-packages/"
endif
ifeq ($(OS),osx)
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC)" LDSHARED="$(CC) -shared" LDFLAGS="$(LDFLAGS) -lpython" PYTHONPATH="$(PREFIX)/lib/python2.7/site-packages/"
endif
ifeq ($(OS),darwin_embedded)
PYTHON_O=$(abs_top_srcdir)/target/python27/$(PLATFORM)/Modules/python.o
CROSSFLAGS=PYTHONXCPREFIX="$(PREFIX)" CC="$(CC) $(CFLAGS)" CCSHARED="$(CC) $(CFLAGS) $(PYTHON_O)" LDFLAGS="$(LDFLAGS)" PYTHONPATH="$(PREFIX)/lib/python2.7/site-packages/"
endif

LIBDYLIB=$(PLATFORM)/dist/$(LIBNAME)-$(VERSION)-py2.7.egg

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py build_ext --plat-name $(OS)-$(CPU)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
