include ../../Makefile.include APACHE-ANT-VERSION ../../download-files.include
DEPS = ../../Makefile.include APACHE-ANT-VERSION Makefile ../../download-files.include


LIBDYLIB=$(PLATFORM)/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	rm -rf $(PREFIX)/share/ant; mkdir -p $(PREFIX)/share/ant
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); cp -rp ./* $(PREFIX)/share/ant/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
