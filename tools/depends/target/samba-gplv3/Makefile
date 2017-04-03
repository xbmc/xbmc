include ../../Makefile.include
DEPS= ../../Makefile.include Makefile configureEndian.patch samba_android.patch samba_off64_t.patch no_fork_and_exec.patch

# lib name, version
LIBNAME=samba
VERSION=4.0.26
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

ifeq ($(OS), ios)
ifeq ($(CPU), arm64)
# clang issue with optimizing out functions
CONFIGURE_EXTRAS= --without-libmsrpc samba_cv_optimize_out_funcation_calls=no ac_cv_func_yp_get_default_domain=no
endif
endif

CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
          --without-cluster-support --without-ldap \
          --without-pam --without-pam_smbpass --enable-fhs \
          --without-winbind --disable-cups --without-ads \
          --disable-avahi \
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

LIBDYLIB=$(PLATFORM)/source3/bin/libsmbclient.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 -i ../configureEndian.patch
	cd $(PLATFORM)/source3; ./autogen.sh
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
	cd $(PLATFORM)/source3; $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/source3 libsmbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/source3 installlibsmbclient
ifeq (darwin, $(findstring darwin, $(HOST)))
	mv $(PREFIX)/lib/libsmbclient.dylib.0 $(PREFIX)/lib/libsmbclient.dylib
	install_name_tool -id $(PREFIX)/lib/libsmbclient.dylib $(PREFIX)/lib/libsmbclient.dylib
endif
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/source3 clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

