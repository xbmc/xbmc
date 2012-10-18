include ../Makefile.include
DEPS= ../Makefile.include Makefile samba30-3.0.37-configure.in.patch \
      samba30-3.0.37-CVE-2010-2063.patch \
      samba30-3.0.37-silence-receive-warning.patch \
      samba30-3.0.37-vfs_default.c.patch samba30-3.0.37-wle-fix.patch \
      samba_android.patch

# lib name, version
LIBNAME=samba
VERSION=3.0.37
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
# android does not really support LFS but we can force it
# in libsmbclient by defining off_t to off64_t and changing
# libsmbclient.h to match. Defining _OFF_T_DEFINED_ allows
# us to change the typedef of off_t in android's sys/type.h.
# Then xbmc will correctly access smb shares with LFS enabled
# using the altered libsmbclient.h.
export CFLAGS+=-D_FILE_OFFSET_BITS=64 -D_OFF_T_DEFINED_ -Doff_t=off64_t -Dlseek=lseek64
CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) --host=$(HOST) \
        SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes \
        libreplace_cv_READDIR_GETDIRENTRIES=no \
        libreplace_cv_READDIR_GETDENTS=no \
        linux_getgrouplist_ok=no \
        samba_cv_REPLACE_READDIR=no \
        samba_cv_HAVE_WRFILE_KEYTAB=yes \
        samba_cv_HAVE_GETTIMEOFDAY_TZ=yes \
        samba_cv_USE_SETREUID=yes \
        samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes \
        samba_cv_HAVE_IFACE_IFCONF=yes \
        samba_cv_HAVE_MMAP=yes \
        samba_cv_HAVE_FCNTL_LOCK=yes \
        samba_cv_HAVE_SECURE_MKSTEMP=yes \
        samba_cv_CC_NEGATIVE_ENUM_VALUES=yes \
        samba_cv_fpie=no \
        samba_cv_have_longlong=yes \
        samba_cv_HAVE_OFF64_T=yes \
	samba_cv_HAVE_UT_UT_TYPE=no \
        ac_cv_func_srand=yes \
        ac_cv_func_rand=yes \
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

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 < ../samba30-3.0.37-configure.in.patch
	cd $(PLATFORM); patch -p1 < ../samba30-3.0.37-CVE-2010-2063.patch
	cd $(PLATFORM); patch -p1 < ../samba30-3.0.37-silence-receive-warning.patch
	cd $(PLATFORM); patch -p1 < ../samba30-3.0.37-vfs_default.c.patch
	cd $(PLATFORM); patch -p1 < ../samba30-3.0.37-wle-fix.patch
	cd $(PLATFORM); patch -p0 < ../samba_android.patch
	sed -ie "s|off_t|off64_t|g" "$(PLATFORM)/source/include/libsmbclient.h"
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

