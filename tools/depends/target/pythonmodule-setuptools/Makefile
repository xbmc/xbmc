include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

VERSION.TXT := $(CMAKE_SOURCE_DIR)/version.txt
APP_NAME=$(shell awk '/APP_NAME/ {print tolower($$2)}' $(VERSION.TXT))

# lib name, version
LIBNAME=setuptools
VERSION=53.0.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

PYTHONPATH=PYTHONPATH="$(PYTHON_SITE_PKG)"

LIBDYLIB=$(PLATFORM)/dist/$(LIBNAME)-$(VERSION)-py$(PYTHON_VERSION).egg

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	&& cd $(PLATFORM) && $(PYTHONPATH) cross-python setup.py build_ext

.installed-$(PLATFORM): $(LIBDYLIB)
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	&& cd $(PLATFORM) && $(PYTHONPATH) cross-python setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
