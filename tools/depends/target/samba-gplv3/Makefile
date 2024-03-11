include ../../Makefile.include SAMBA-GPLV3-VERSION ../../download-files.include
DEPS= ../../Makefile.include Makefile SAMBA-GPLV3-VERSION ../../download-files.include \
                              01-fix-dependencies.patch 02-cross_compile.patch \
                              03-builtin-heimdal.patch 04-built-static.patch \
                              05-apple-disable-zlib-pkgconfig.patch 06-apple-fix-st_atim.patch \
                              samba_android.patch \
                              no_fork_and_exec.patch \
                              crt_extensions.patch \

# vfs_snapper requires dbus, so disable.
CONFIGURE=./configure --prefix=$(PREFIX) \
          --cross-compile --cross-answers=cross-answers.txt \
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
          --builtin-libraries=!smbclient,!smbd_base,!smbstatus,ALL \
          --with-shared-modules=!vfs_snapper \
          --disable-python \
          --disable-symbol-versions \
          --without-json \
          --without-libarchive \
          --without-regedit \
          --without-lttng

LIBDYLIB=$(PLATFORM)/bin/default/source3/libsmb/libsmbclient.a

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

# samba doesn't build with distcc
export DISTCC_HOSTS=

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../01-fix-dependencies.patch
	cd $(PLATFORM); patch -p1 -i ../02-cross_compile.patch
	cd $(PLATFORM); patch -p1 -i ../03-builtin-heimdal.patch
	cd $(PLATFORM); patch -p1 -i ../04-built-static.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 -i ../samba_android.patch
	cd $(PLATFORM); patch -p1 -i ../07-innetgr-check.patch
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

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); WAF_MAKE=1 ./buildtools/bin/waf --targets=smbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); WAF_MAKE=1 ./buildtools/bin/waf install --targets=smbclient
	install $(LIBDYLIB) $(PREFIX)/lib
	touch $@

clean:
	cd $(PLATFORM); WAF_MAKE=1 ./buildtools/bin/waf uninstall --targets=smbclient
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
