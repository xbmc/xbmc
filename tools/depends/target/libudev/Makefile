include ../../Makefile.include LIBUDEV-VERSION ../../download-files.include
DEPS =../../Makefile.include LIBUDEV-VERSION Makefile ../../download-files.include

# configuration settings
# the static libudev exports eudev's internal helpers; rename those that
# clash with other static libs such as libsmbclient
export CFLAGS+=-Dparse_time=eudev_parse_time -Dstrv_find=eudev_strv_find
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --enable-static --enable-introspection=no --disable-programs --disable-blkid --disable-selinux --disable-manpages --disable-kmod --disable-mtd_probe

LIBDYLIB=$(PLATFORM)/src/libudev/.libs/libudev.la

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
