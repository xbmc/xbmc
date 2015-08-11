include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=libuuid
VERSION=2.20.0
SOURCE=util-linux-2.20.1
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=echo "scanf_cv_type_modifier=as" > config.cache; \
	  ./configure --prefix=$(PREFIX) --cache-file=config.cache \
	              --disable-shared --disable-mount --disable-fsck --disable-partx --disable-libblkid \
                      --disable-libmount --disable-mountpoint --disable-nls --disable-rpath --disable-agetty \
                      --disable-cramfs --disable-switch_root --disable-pivot_root --disable-fallocate \
                      --disable-unshare --disable-rename --disable-schedutils --disable-wall --without-ncurses

LIBDYLIB=$(PLATFORM)/libuuid/src/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/libuuid

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/libuuid install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/libuuid clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
