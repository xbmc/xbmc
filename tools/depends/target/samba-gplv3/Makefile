include ../../Makefile.include SAMBA-GPLV3-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile SAMBA-GPLV3-VERSION ../../download-files.include \
                              01-fix-dependencies.patch 02-cross_compile.patch \
                              03-builtin-heimdal.patch 04-built-static.patch \
                              05-apple-disable-zlib-pkgconfig.patch 06-apple-fix-st_atim.patch \
                              samba_android.patch \
                              no_fork_and_exec.patch \
                              crt_extensions.patch

# vfs_snapper requires dbus, so disable.
CONFIGURE=./configure --prefix=$(PREFIX) \
          --cross-compile --cross-answers=cross-answers.txt \
          --hostcc="$(CC_FOR_BUILD) $(NATIVE_CFLAGS)" \
          --without-cluster-support \
          --without-ldap \
          --without-pam \
          --enable-fhs \
          --without-winbind \
          --without-ads \
          --disable-avahi \
          --disable-cups \
          --without-gettext \
          --without-ad-dc \
          --without-acl-support \
          --without-utmp \
          --disable-iprint \
          --nopyc --nopyo \
          --builtin-libraries=!smbclient,!smbd_base,!smbstatus,!ldb,!samba-hostconfig,!samdb,!samdb-common,!authkrb5,!netif,!ldbsamba,!stable_sort,!cli-ldap,!samba-security,!dcerpc,!auth_unix_token,!smbclient-raw,ALL \
          --with-shared-modules=!vfs_snapper \
          --disable-python \
          --disable-symbol-versions \
          --without-json \
          --without-libarchive \
          --without-regedit \
          --disable-fault-handling \
          --disable-wsp \
          --without-lttng

ifeq ($(NEED_LIBICONV), 1)
  CONFIGURE += --with-libiconv=$(PREFIX)/lib
endif

LIBDYLIB=$(PLATFORM)/bin/default/source3/libsmb/libsmbclient.a
HEIMDAL_ASN1_OBJS=$(wildcard $(PLATFORM)/bin/default/third_party/heimdal/lib/asn1/asn1_*_asn1*.o) \
                  $(wildcard $(PLATFORM)/bin/default/third_party/heimdal/lib/gssapi/asn1_*_asn1*.o) \
                  $(wildcard $(PLATFORM)/bin/default/third_party/heimdal/lib/hx509/asn1_*_asn1*.o)

ifeq ($(OS), darwin_embedded)
  # build errors with _yp_get_default_domain NIS failure
  CFLAGS+= -Wno-error=implicit-function-declaration
endif

ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
  # perlmodule-parseyapp install location for darwin host
  PERLMODULE=$(NATIVEPREFIX)/lib/perl5/site_perl
else
  ifeq ($(findstring -linux-, $(HOST)), -linux-)
    # linux host installs to $NATIVEPREFIX/share with perl version as folder
    # just do a find for the Parse folder to handle variable versions
    location:=$(shell find $(NATIVEPREFIX)/share/ -type d -name Parse | sed 's|$(NATIVEPREFIX)/||' | sed 's|/Parse||')
    PERLMODULE=$(NATIVEPREFIX)/$(location)
  endif
endif

# LLVM 15 has raised this to error by default. drop back to warning
CFLAGS+= -Wno-error=int-conversion

export PERL5LIB:=$(PERLMODULE):$(PERL5LIB)

export CC CXX CPP AR RANLIB LD AS NM STRIP TOOLCHAIN
export CFLAGS CXXFLAGS CPPFLAGS LDFLAGS
export HOST
export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig
export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig

# samba doesn't build with distcc
export DISTCC_HOSTS=

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-fix-dependencies.patch
	cd $(PLATFORM); patch -p1 -i ../02-cross_compile.patch
	cd $(PLATFORM); patch -p1 -i ../03-builtin-heimdal.patch
	cd $(PLATFORM); patch -p1 -i ../04-built-static.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../samba_android.patch
endif
ifeq ($(OS), darwin_embedded)
	cd $(PLATFORM); patch -p1 -i ../no_fork_and_exec.patch
	cd $(PLATFORM); patch -p1 -i ../crt_extensions.patch
endif
ifeq ($(findstring apple-darwin, $(HOST)), apple-darwin)
	cd $(PLATFORM); patch -p1 -i ../05-apple-disable-zlib-pkgconfig.patch
	cd $(PLATFORM); patch -p1 -i ../06-apple-fix-st_atim.patch
endif
ifeq ($(TARGET_PLATFORM),webos)
	cd $(PLATFORM); patch -p1 -i ../webos-no-readline.patch
endif
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	cd $(PLATFORM); PYTHONHASHSEED=1 WAF_MAKE=1 ./buildtools/bin/waf --targets=smbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); PYTHONHASHSEED=1 WAF_MAKE=1 ./buildtools/bin/waf install --targets=smbclient
	install $(LIBDYLIB) $(PREFIX)/lib
	$(AR) -q $(PREFIX)/lib/libsmbclient.a $(HEIMDAL_ASN1_OBJS)
	$(RANLIB) $(PREFIX)/lib/libsmbclient.a
	touch $@

clean:
	cd $(PLATFORM); PYTHONHASHSEED=1 WAF_MAKE=1 ./buildtools/bin/waf uninstall --targets=smbclient
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
