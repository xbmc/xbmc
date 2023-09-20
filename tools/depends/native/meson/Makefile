include ../../Makefile.include MESON-VERSION ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile MESON-VERSION ../../download-files.include

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py config

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py build
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install --prefix="$(NATIVEPREFIX)"
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
