include ../Makefile.include
DEPS= ../Makefile.include 01-gcrypt.patch 02-pointer.patch 04-boxee1.patch \
      05-boxee2.patch 06-boxee3.patch 07-boxee4.patch \
      08-boxee5.patch 10-fix-errno.patch 11-fix-stat.patch \
      android.patch fix_afpfs-ng_includes.patch Makefile

# lib name, version
LIBNAME=afpfs-ng
VERSION=0.8.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH) 
export LIBS=-liconv
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) \
  --disable-fuse ac_cv_func_malloc_0_nonnull=yes

LIBDYLIB=$(PLATFORM)/lib/.libs/libafpclient.so.0.0.0

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 <../fix_afpfs-ng_includes.patch
	cd $(PLATFORM); patch -p1 <../01-gcrypt.patch
	cd $(PLATFORM); patch -p1 <../02-pointer.patch
	cd $(PLATFORM); patch -p1 <../04-boxee1.patch
	cd $(PLATFORM); patch -p1 <../05-boxee2.patch	
	cd $(PLATFORM); patch -p1 <../06-boxee3.patch
	cd $(PLATFORM); patch -p1 <../07-boxee4.patch
	cd $(PLATFORM); patch -p1 <../08-boxee5.patch
	cd $(PLATFORM); patch -p1 <../10-fix-errno.patch
	cd $(PLATFORM); patch -p1 <../11-fix-stat.patch
	cd $(PLATFORM); patch -p0 < ../android.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)/lib

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/lib install
	$(MAKE) -C $(PLATFORM)/include install
	rm -f $(PREFIX)/lib/libafpclient.la $(PREFIX)/lib/libafpclient.so $(PREFIX)/lib/libafpclient.so.0
	mv -f $(PREFIX)/lib/libafpclient.so.0.0.0 $(PREFIX)/lib/libafpclient.so
	$(RPL) -e "libafpclient.so.0" "libafpclient.so\x00\x00" $(PREFIX)/lib/libafpclient.so
	-$(READELF) --dynamic $(PREFIX)/lib/libafpclient.so | grep ibrary
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
