include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=mtdev
VERSION=1.1.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=78477a4af784e3f846df867f1428ffdc458e3ff33d465d45a84247a7e52f0e215cace397d72be64f6e641f07e0b97a930eff0e07fe9507e0138fb028a4037d6d
include ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) config-aux/; ./configure --prefix=$(PREFIX) --enable-static --disable-shared

LIBDYLIB=$(PLATFORM)/src/.libs/libmtdev.la

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
