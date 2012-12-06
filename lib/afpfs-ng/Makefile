#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# lib name, version
LIBNAME=afpfs-ng
VERSION=0.8.1
SOURCE=$(LIBNAME)-$(VERSION)

# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.gz.
TARBALLS_LOCATION=.
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf
PREFIX ?= /usr/local
AFPFSNG_CONFIGOPTS ?= --prefix=$(PREFIX)


ARCHIVE=$(SOURCE).tar.bz2



# configuration settings
export PATH:=$(TOOLCHAIN)/bin:$(PATH)
CONFIGURE=./configure $(AFPFSNG_CONFIGOPTS) \
  --disable-fuse ac_cv_func_malloc_0_nonnull=yes

SO_NAME=$(SOURCE)/lib/.libs/libafpclient.so

all: $(SO_NAME)

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
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -j 1 -C $(SOURCE)

install:
	make -C $(SOURCE) install

clean:
	make -C $(SOURCE) clean

distclean::
	rm -rf $(SOURCE) $(ARCHIVE)
