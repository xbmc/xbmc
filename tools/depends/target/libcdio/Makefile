include ../../Makefile.include
DEPS = ../../Makefile.include Makefile configure.patch cross.patch ../../download-files.include

# lib name, version
LIBNAME=libcdio
VERSION=0.80
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=4bc723fd882baac090f61576aa798a3045df7877c9482b81e0c78e25e29503774240b7c34d255150ca7557d9aa8d8b113ca6fd1c1dae1362e4c0f314d8f33a51
include ../../download-files.include
# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX)  --with-cd-drive=no --with-cd-info=no --with-cd-paranoia=no \
          --with-cdda-player=no --with-cd-read=no --with-iso-info=no --with-iso-read=no --disable-example-progs \
          --disable-cpp-progs --disable-cxx --disable-shared

LIBDYLIB=$(PLATFORM)/lib/driver/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../configure.patch
	cd $(PLATFORM); patch -p1 -i ../cross.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/lib install
	$(MAKE) -C $(PLATFORM)/include install
	$(MAKE) -C $(PLATFORM) install-data-am
	cp $(PLATFORM)/include/cdio/cdtext.h $(PREFIX)/include/cdio/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

