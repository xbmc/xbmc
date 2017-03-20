include ../../Makefile.include
DEPS= ../../Makefile.include Makefile syntax.patch samba_android.patch samba_off64_t.patch no_fork_and_exec.patch

# lib name, version
LIBNAME=samba
VERSION=4.5.3
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
# will remove befor merge
BASE_URL=https://download.samba.org/pub/samba/stable

ifeq ($(OS), ios)
ifeq ($(CPU), arm64)
# clang issue with optimizing out functions
CONFIGURE_EXTRAS= --without-libmsrpc samba_cv_optimize_out_funcation_calls=no ac_cv_func_yp_get_default_domain=no
endif
endif

CONFIGURE=./configure -v --prefix=$(PREFIX) \
          --without-cluster-support --without-ldap \
          --without-pam --enable-fhs \
          --without-winbind --disable-cups --without-ads \
          --disable-avahi --without-gettext \
          --without-dnsupdate \
          --without-ad-dc --without-acl-support \
          --with-libiconv=$(STAGING_DIR)

# configuration settings
# android does not really support LFS but we can force it
# in libsmbclient by defining off_t to off64_t and changing
# libsmbclient.h to match. Defining _OFF_T_DEFINED_ allows
# us to change the typedef of off_t in android's sys/type.h.
# Then xbmc will correctly access smb shares with LFS enabled
# using the altered libsmbclient.h.
ifeq ($(OS),android)
ifeq ($(findstring arm64, $(CPU)), arm64)
export CFLAGS+=-DHAVE_BZERO
endif
endif

LIBDYLIB=$(PLATFORM)/bin/default/source3/libsmb/libsmbclient.so
ifeq ($(OS),$(filter $(OS), osx ios))
LIBDYLIB=$(PLATFORM)/bin/default/source3/libsmb/libsmbclient.dylib
endif

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

export CC CXX CPP AR RANLIB LD AS NM STRIP TOOLCHAIN
export CFLAGS CXXFLAGS CPPFLAGS
export LDFLAGS := $(NATIVE_LDFLAGS) $(LDFLAGS)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../syntax.patch
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p0 < ../samba_android.patch
	cd $(PLATFORM); patch -p1 < ../samba_off64_t.patch
endif
ifeq ($(TARGET_PLATFORM),appletvos)
	cd $(PLATFORM); patch -p0 < ../no_fork_and_exec.patch
endif
ifeq (darwin, $(findstring darwin, $(HOST)))
	sed -ie "s|ifndef HAVE_CLOCK_GETTIME|if !defined(HAVE_CLOCK_GETTIME) \&\& !defined(CLOCK_REALTIME)|" "$(PLATFORM)/lib/replace/system/time.h"
endif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); WAF_MAKE=1 $(NATIVEPREFIX)/bin/python ./buildtools/bin/waf -v --targets=smbclient

.installed-$(PLATFORM): $(LIBDYLIB)
ifeq (darwin, $(findstring darwin, $(HOST)))
	cd $(PLATFORM)/bin/default; for file in $$(find * -iregex '.*\.dylib'); do link $$file $$(echo $$file|sed 's/.dylib$$/.inst.dylib/g') 2>/dev/null; done
else
	cd $(PLATFORM)/bin/default; for file in $$(find * -iregex '.*\.so'); do link $$file $$(echo $$file|sed 's/.so$$/.inst.so/g') 2>/dev/null; done
endif
	cd $(PLATFORM); WAF_MAKE=1 $(NATIVEPREFIX)/bin/python ./buildtools/bin/waf install --targets=smbclient
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libsmbclient.so
	mv -f $(PREFIX)/lib/libsmbclient.so.0 $(PREFIX)/lib/libsmbclient.so
	$(RPL) -e "libsmbclient.so.0" "libsmbclient.so\x00\x00" $(PREFIX)/lib/libsmbclient.so
	-$(READELF) --dynamic $(PREFIX)/lib/libsmbclient.so | grep ibrary
endif
	touch $@

clean:
	cd $(PLATFORM); WAF_MAKE=1 $(NATIVEPREFIX)/bin/python ./buildtools/bin/waf uninstall --targets=smbclient
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

