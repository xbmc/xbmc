include ../../Makefile.include GIFLIB-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include GIFLIB-VERSION Makefile ../../download-files.include \
                          01-static-lib.patch

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/libgif.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	-rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-static-lib.patch

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM) libgif.a CFLAGS="$(NATIVE_CFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)"

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install-include PREFIX="$(PREFIX)"
	$(MAKE) -C $(PLATFORM) install-lib PREFIX="$(PREFIX)"
	touch $@
clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -r .installed-$(PLATFORM)
	rm -rf $(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
