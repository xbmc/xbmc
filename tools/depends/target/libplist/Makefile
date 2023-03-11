include ../../Makefile.include
DEPS = ../../Makefile.include Makefile 01-disable-tools-tests.patch ../../download-files.include

# lib name, version
LIBNAME=libplist
VERSION=2.2.0
SOURCE=$(LIBNAME)-$(VERSION)
FILENAME=v$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=dcd922ebb28efa3c507253dd4671fdb3e4d367540734765b8d545756f6be2731fc401e7992cc6b06cdbde866fd7e17ff6188895505988b41f738226a004d7291
include ../../download-files.include
LIBDYLIB=$(PLATFORM)/src/.libs/libplist++-2.0.a
CFLAGS+=-fvisibility=default
all: .installed-$(PLATFORM)

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; ./configure --prefix=$(PREFIX) --disable-shared --without-cython


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-disable-tools-tests.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
