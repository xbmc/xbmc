#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# lib name, version
LIBNAME=libnfs
VERSION=0804e67
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.gz
TARBALLS_LOCATION=.
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf
PREFIX ?= /usr
LIBNFS_CONFIGOPTS ?= --prefix=$(PREFIX)
# configuration settings

CONFIGURE=./configure CFLAGS=-D_FILE_OFFSET_BITS=64 $(LIBNFS_CONFIGOPTS)

SO_NAME=$(SOURCE)/lib/libnfs.so

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p0 < ../001_fix_hanewin.patch
	cd $(SOURCE);./bootstrap;$(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -C $(SOURCE)

install:
	make -C $(SOURCE) install
clean:
	make -C $(SOURCE) clean
	rm  -f .installed
	rm -f -r include
	rm -f -r lib
	rm -f *.tar.gz
	rm -r -f nfsc

distclean::
	rm -rf $(SOURCE) .installed
