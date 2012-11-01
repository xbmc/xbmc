include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=afpfs-ng
VERSION=0.8.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH) 
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-fuse ac_cv_func_malloc_0_nonnull=yes

LIBDYLIB=$(SOURCE)/lib/.libs/libafpclient.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p1 <../fix_afpfs-ng_includes.patch
	cd $(SOURCE); patch -p1 <../01-gcrypt.patch
	cd $(SOURCE); patch -p1 <../02-pointer.patch
	cd $(SOURCE); patch -p1 <../04-boxee1.patch
	cd $(SOURCE); patch -p1 <../05-boxee2.patch	
	cd $(SOURCE); patch -p1 <../06-boxee3.patch
	cd $(SOURCE); patch -p1 <../07-boxee4.patch
	cd $(SOURCE); patch -p1 <../08-boxee5.patch
	cd $(SOURCE); patch -p1 <../10-fix-errno.patch
	cd $(SOURCE); patch -p1 <../11-fix-stat.patch
	cd $(SOURCE); patch -p1 <../12-fix-even-more-pointers.patch
	sed -ie 's|SUBDIRS = lib cmdline include docs|SUBDIRS = lib include|' $(SOURCE)/Makefile.am
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
