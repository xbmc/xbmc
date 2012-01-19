#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# lib name, version
LIBNAME=libshairport
VERSION=1.2.0.20310_lib
SOURCE=$(LIBNAME)-$(VERSION)

# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.gz 
TARBALLS_LOCATION=.
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

PREFIX ?= /usr/local
LIBSHAIRPORT_CONFIGOPTS ?= --prefix=$(PREFIX)

# configuration settings
CONFIGURE=./configure $(LIBSHAIRPORT_CONFIGOPTS)


SO_NAME=$(SOURCE)/.libs/$(LIBNAME).so

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p0 < ../001_add_ao.patch
	cd $(SOURCE); patch -p0 < ../002_fix_install_header.patch
	cd $(SOURCE); patch -p0 < ../003_fix_deadlock.patch
	cd $(SOURCE); patch -p0 < ../004_fix_bad_access.patch
	cd $(SOURCE); patch -p0 < ../005_fix_shutdown.patch
	cd $(SOURCE); patch -p0 < ../006_no_printf.patch
	cd $(SOURCE); patch -p0 < ../007_fix_syslog_defines.patch
	cd $(SOURCE); patch -p0 < ../008-add-missing-libs.patch
	cd $(SOURCE); patch -p0 < ../009_fix_ipv6.patch
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -j 1 -C $(SOURCE)

install:
	make -C $(SOURCE) install
	ldconfig

clean:
	rm -rf $(SOURCE)
	#make -C $(SOURCE) clean

distclean::
	rm -rf $(SOURCE)
