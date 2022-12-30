include ../../Makefile.include
DEPS = ../../Makefile.include Makefile osx.patch 01-fix-glob-on-android.patch ../../download-files.include

# lib name, version
LIBNAME=libcdio
VERSION=2.1.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
SHA512=c290821da55fd9ae366670a58857aa6efcebc9f25b7caea063cf12f9cbda84fe770c5f59f972227fda50517ca58c5f39c0137daa0f93179e3daa45303d8b610f
include ../../download-files.include

# configuration settings
CONFIGURE=cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX)  --with-cd-drive=no --with-cd-info=no \
          --with-cdda-player=no --with-cd-read=no --with-iso-info=no --with-iso-read=no --disable-example-progs \
          --disable-cpp-progs --enable-cxx --disable-shared

LIBDYLIB=$(PLATFORM)/lib/driver/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(TARGET_PLATFORM),macosx)
	cd $(PLATFORM); patch -p1 -i ../osx.patch
endif
	cd $(PLATFORM); patch -p1 -i ../01-fix-glob-on-android.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/lib install
	$(MAKE) -C $(PLATFORM)/include install
	$(MAKE) -C $(PLATFORM) install-data-am
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
