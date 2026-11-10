include ../../Makefile.include TAR-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include TAR-VERSION Makefile ../../download-files.include 01-link-libintl-libiconv.patch

export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --program-transform-name=s/tar/gtar/   \
  --disable-dependency-tracking \
  $(PLATFORM_CONFIGURE)

APP=$(PLATFORM)/src/tar
APPBIN=$(PREFIX)/bin/tar

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-link-libintl-libiconv.patch
	cd $(PLATFORM); $(AUTORECONF)
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(APP): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/bin/tar $(PREFIX)/bin/gnutar
	cd $(PREFIX)/bin; ln -s gtar tar; ln -s gtar gnutar
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
