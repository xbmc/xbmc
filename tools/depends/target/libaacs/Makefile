include ../../Makefile.include LIBAACS-VERSION ../../download-files.include
DEPS = ../../Makefile.include LIBAACS-VERSION Makefile ../../download-files.include \
                    01-all-remove_versioning.patch \
                    02-all-AACS_HOME-env.patch

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
                      --disable-static \
                      --exec-prefix=$(PREFIX)

ifeq ($(OS),osx)
  BYPRODUCT=libaacs.dylib
else
  BYPRODUCT=libaacs.so
endif

LIBDYLIB=$(PLATFORM)/.libs/$(BYPRODUCT)

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-all-remove_versioning.patch
	cd $(PLATFORM); patch -p1 -i ../02-all-AACS_HOME-env.patch
	cd $(PLATFORM); ./bootstrap
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
