include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=setuptools
VERSION=18.4
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PYTHONPATH=$(PYTHON_SITE_PKG)

# TODO - remove for android/osx
ifeq ($(OS),android)
  LDSHARED="$(CC) -shared"
  CC+=" $(CFLAGS)"
endif
ifeq ($(OS),osx)
  LDSHARED="$(CC) -shared"
  LDFLAGS+=" -lpython"
endif

export LDSHARED LDFLAGS CC PYTHONPATH

LIBDYLIB=$(PLATFORM)/dist/$(LIBNAME)-$(VERSION)-py$(PYTHON_VERSION).egg

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	mkdir -p $(PLATFORM)/output
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py build_ext

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(CROSSFLAGS) $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
