include ../Makefile.include
DEPS= ../Makefile.include 001_add_ao.patch 002_fix_install_header.patch 003_fix_deadlock.patch \
      004_fix_bad_access.patch 005_fix_shutdown.patch 006_no_printf.patch \
      007_fix_syslog_defines.patch 008-add-missing-libs.patch \
      009_fix_ipv6.patch 010_handle_metadata.patch android.patch Makefile

# lib name, version
LIBNAME=libshairport
VERSION=1.2.0.20310_lib
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)


LIBDYLIB=$(PLATFORM)/src/.libs/libshairport.so.0.0.0

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p0 < ../001_add_ao.patch
	cd $(PLATFORM); patch -p0 < ../002_fix_install_header.patch
	cd $(PLATFORM); patch -p0 < ../003_fix_deadlock.patch
	cd $(PLATFORM); patch -p0 < ../004_fix_bad_access.patch
	cd $(PLATFORM); patch -p0 < ../005_fix_shutdown.patch
	cd $(PLATFORM); patch -p0 < ../006_no_printf.patch
	cd $(PLATFORM); patch -p0 < ../007_fix_syslog_defines.patch
	cd $(PLATFORM); patch -p0 < ../008-add-missing-libs.patch
	cd $(PLATFORM); patch -p0 < ../009_fix_ipv6.patch
	cd $(PLATFORM); patch -p0 < ../010_handle_metadata.patch
	cd $(PLATFORM); patch -p0 < ../011_fix_ipv4_fallback.patch
	cd $(PLATFORM); patch -p0 < ../android.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	rm -f $(PREFIX)/lib/libshairport.la $(PREFIX)/lib/libshairport.so $(PREFIX)/lib/libshairport.so.0
	mv -f $(PREFIX)/lib/libshairport.so.0.0.0 $(PREFIX)/lib/libshairport.so
	$(RPL) -e "libshairport.so.0" "libshairport.so\x00\x00" $(PREFIX)/lib/libshairport.so
	-$(READELF) --dynamic $(PREFIX)/lib/libshairport.so | grep ibrary
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

