include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 02-android-dlopen.patch

# lib name, version
LIBNAME=pycryptodome
VERSION=3.4.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../02-android-dlopen.patch
endif

$(PLATFORM)/.separate_namespace: $(PLATFORM)
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	  && cd $(PLATFORM); touch ../$@ && cross-python setup.py build_ext --plat-name $(OS)-$(TARGET_ARCH)
	touch $@

.installed-$(PLATFORM): $(PLATFORM)/.separate_namespace
	. $(NATIVEPREFIX)/bin/$(PYTHON_TARGETENV)/bin/activate \
	  && cd $(PLATFORM); cross-python setup.py install --prefix=$(PREFIX)
# Setuptools installs the egg into a pycryptodomex-$(version)-*.egg folder
# Move the Cryptodome folder back to $(PYTHON_SITE_PKG) to match distutils install
	mv $(PYTHON_SITE_PKG)/pycryptodomex*/Cryptodome $(PYTHON_SITE_PKG)/ \
	  && rm -rf $(PYTHON_SITE_PKG)/pycryptodomex*
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
