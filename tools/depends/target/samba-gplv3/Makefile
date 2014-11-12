include ../../Makefile.include
DEPS= ../../Makefile.include \
      Makefile \
      samba_crypto_osx.patch \
      samba_string_fix.patch \


# lib name, version
LIBNAME=samba
VERSION=4.1.13
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
          --without-cluster-support --without-ldap \
          --without-pam --without-pam_smbpass --enable-fhs \
          --without-winbind --disable-cups --without-ads \
          --disable-avahi --without-fam \
          --without-dnsupdate \
	  --without-ldap \
	  --disable-iprint \
	  --without-sendfile-support \
	  --without-acl-support \
	  --without-automount \
	  --without-aio-support \
	  --without-dmapi \
	  --disable-glusterfs \
	  --without-ad-dc \
	  --without-systemd


LIBDYLIB=$(PLATFORM)/source3/bin/libsmbclient.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

.configured-$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../samba_crypto_osx.patch
	cd $(PLATFORM); patch -p1 < ../samba_string_fix.patch
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM) bin/shared/libsmbclient.0.dylib

.installed-$(PLATFORM): $(LIBDYLIB)
	cp $(PLATFORM)/bin/shared/libsmbclient.0.dylib $(PREFIX)/lib/libsmbclient.dylib.0
	cp $(PLATFORM)/bin/default/include/public/libsmbclient.h $(PREFIX)/include/libsmbclient.h
ifeq (darwin, $(findstring darwin, $(HOST)))
	install_name_tool -id $(PREFIX)/lib/libsmbclient.dylib.0 $(PREFIX)/lib/libsmbclient.dylib.0
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)
	rm -f .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

