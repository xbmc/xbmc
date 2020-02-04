include ../../Makefile.include
DEPS= ../../Makefile.include Makefile 01-fix-dependencies.patch 02-cross_compile.patch 03-builtin-heimdal.patch 04-built-static.patch samba_android.patch no_fork_and_exec.patch crt_extensions.patch

# lib name, version
LIBNAME=samba
VERSION=4.11.6
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

CONFIGURE=./configure --prefix=$(PREFIX) \
          --cross-compile --cross-answers=cross-answers.txt \
          --without-cluster-support --without-ldap \
          --without-pam --enable-fhs \
          --without-winbind --disable-cups --without-ads \
          --disable-avahi --without-gettext \
          --without-dnsupdate \
          --without-ad-dc --without-acl-support \
          --without-utmp --disable-iprint \
          --nopyc --nopyo \
          --builtin-libraries=!smbclient,!smbd_base,!smbstatus,ALL \
          --disable-python \
          --disable-symbol-versions \
          --without-json \
          --without-libarchive \
          --without-regedit

LIBDYLIB=$(PLATFORM)/bin/default/source3/libsmb/libsmbclient.a

export CC CXX CPP AR RANLIB LD AS NM STRIP TOOLCHAIN
export CFLAGS CXXFLAGS CPPFLAGS LDFLAGS
export HOST
export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
	cd $(PLATFORM); $(CONFIGURE)
ifeq ($(TARGET_PLATFORM), iphoneos)
	# clock_gettime is only available since macOS 10.12, iOS 10.0 and tvOS 10.0
	# build sytem doesn't include time.h for the check,
	# so the version guards have no effect
	sed -ie -E "s/#define (HAVE_(CLOCK_GETTIME|CLOCK_MONOTONIC|CLOCK_PROCESS_CPUTIME_ID|CLOCK_REALTIME)) 1/\/* #undef \1 *\//g" $(PLATFORM)/bin/default/include/config.h
endif

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
