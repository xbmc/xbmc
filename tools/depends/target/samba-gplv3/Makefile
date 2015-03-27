include ../../Makefile.include
DEPS= ../../Makefile.include Makefile samba_android.patch

# lib name, version
LIBNAME=samba
VERSION=3.6.12
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
          --without-cluster-support --disable-swat --without-ldap \
          --without-pam --without-pam_smbpass --with-fhs --with-libtalloc=no \
          --with-libtdb=no --without-winbind --disable-cups --without-ads \
          --disable-avahi --disable-fam --without-libaddns --without-libnetapi \
          --without-dnsupdate --without-libsmbsharemodes \
          --with-libiconv=$(STAGING_DIR)

# configuration settings
# android does not really support LFS but we can force it
# in libsmbclient by defining off_t to off64_t and changing
# libsmbclient.h to match. Defining _OFF_T_DEFINED_ allows
# us to change the typedef of off_t in android's sys/type.h.
# Then xbmc will correctly access smb shares with LFS enabled
# using the altered libsmbclient.h.
ifeq ($(OS),android)
export CFLAGS+=-D_FILE_OFFSET_BITS=64 -D_OFF_T_DEFINED_ -Doff_t=off64_t -Dlseek=lseek64
endif

LIBDYLIB=$(PLATFORM)/source3/bin/libsmbclient.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p0 < ../samba_android.patch
endif
	cd $(PLATFORM)/source3; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/source3 libsmbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/source3 installlibsmbclient
ifeq (darwin, $(findstring darwin, $(HOST)))
	install_name_tool -id $(PREFIX)/lib/libsmbclient.dylib.0 $(PREFIX)/lib/libsmbclient.dylib.0
endif
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libsmbclient.so 
	mv -f $(PREFIX)/lib/libsmbclient.so.0 $(PREFIX)/lib/libsmbclient.so
	$(RPL) -e "libsmbclient.so.0" "libsmbclient.so\x00\x00" $(PREFIX)/lib/libsmbclient.so
	-$(READELF) --dynamic $(PREFIX)/lib/libsmbclient.so | grep ibrary
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/source3 clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

