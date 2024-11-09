include ../../Makefile.include PYTHONMODULE-SETUPTOOLS-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile PYTHONMODULE-SETUPTOOLS-VERSION ../../download-files.include

LIBDYLIB=$(PLATFORM)/dist/$(LIBNAME)-$(VERSION)-py$(PYTHON_VERSION).egg

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); PYTHONPATH="$(PYTHON_SITE_PKG)" $(NATIVEPREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	find $(PYTHON_SITE_PKG)/*setuptools* -type f -name "*.exe" -exec rm -f {} \;
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
