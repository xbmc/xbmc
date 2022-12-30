include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include
      samba30-3.0.37-configure.in.patch \
      samba30-3.0.37-CVE-2010-2063.patch \
      samba30-3.0.37-silence-receive-warning.patch \
      samba30-3.0.37-vfs_default.c.patch samba30-3.0.37-wle-fix.patch \
      samba_android.patch

# lib name, version
LIBNAME=samba
VERSION=3.0.37
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=7311a38abc271b461086d088a2a1b6ba74dc706c57cc34b18088e344e313e7a65f42ab6155a31f5467bc3f9e7bfb1d554288a1553d25503aed2a636f58868c6d
include ../../download-files.include

# configuration settings
# android does not really support LFS but we can force it
# in libsmbclient by defining off_t to off64_t and changing
# libsmbclient.h to match. Defining _OFF_T_DEFINED_ allows
# us to change the typedef of off_t in android's sys/type.h.
# Then xbmc will correctly access smb shares with LFS enabled
# using the altered libsmbclient.h.
ifeq ($(OS),android)
export CFLAGS+=-D_FILE_OFFSET_BITS=64 -D_OFF_T_DEFINED_ -Doff_t=off64_t -Dlseek=lseek64
ANDROID_OFF_T_FIX=sed -ie "s|off_t|off64_t|g" "$(PLATFORM)/source/include/libsmbclient.h"
endif

CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
        --disable-cups \
        --enable-static \
        --disable-shared \
        --disable-pie \
        --disable-iprint \
        --disable-fam \
        --without-cluster-support \
        --without-cifsupcall \
        --without-ads \
        --without-ldap \
        --with-included-popt \
        --with-included-iniparser \
        --without-sys-quotas \
        --without-krb5 \
        --without-automount \
        --without-sendfile-support \
        --with-libiconv=$(STAGING_DIR) \
        --without-cifsmount \
        --without-winbind \

LIBDYLIB=$(PLATFORM)/source/bin/libsmbclient.a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../samba30-3.0.37-configure.in.patch
	cd $(PLATFORM); patch -p1 -i ../samba30-3.0.37-CVE-2010-2063.patch
	cd $(PLATFORM); patch -p1 -i ../samba30-3.0.37-silence-receive-warning.patch
	cd $(PLATFORM); patch -p1 -i ../samba30-3.0.37-vfs_default.c.patch
	cd $(PLATFORM); patch -p1 -i ../samba30-3.0.37-wle-fix.patch
	cd $(PLATFORM); patch -p1 -i ../samba_android.patch
	$(ANDROID_OFF_T_FIX)
	cd $(PLATFORM)/source && ./autogen.sh
	cd $(PLATFORM)/source; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/source headers
	mkdir -p $(PLATFORM)/source/bin
	$(MAKE) -C $(PLATFORM)/source libsmbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/source installclientlib
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/source clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

