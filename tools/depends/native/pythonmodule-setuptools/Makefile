include ../../Makefile.include PYTHONMODULE-SETUPTOOLS-VERSION ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile PYTHONMODULE-SETUPTOOLS-VERSION ../../download-files.include 01-distutils-flag.patch

LIBDYLIB=$(PLATFORM)/dist/$(LIBNAME)-$(VERSION)-py$(NATIVE_PYTHON_VERSION).egg

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); patch -p1 -i ../01-distutils-flag.patch
	cd $(PLATFORM); PYTHONPATH="$(NATIVEPREFIX)/lib/python${NATIVE_PYTHON_VERSION}/site-packages" $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(NATIVEPREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
