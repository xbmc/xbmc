#  Download and build libCEC
#
#  Usage:
#    make
#    sudo make install
#    make clean

# lib name, version
LIBNAME=libcec
VERSION=1.2.0
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
LIBCEC_CONFIGOPTS ?= --prefix=$(PREFIX)

# configuration settings
CONFIGURE=./configure CFLAGS=-D_FILE_OFFSET_BITS=64 $(LIBCEC_CONFIGOPTS)

SO_NAME=$(SOURCE)/.libs/$(LIBNAME).so

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -j 1 -C $(SOURCE)

install:
	make -C $(SOURCE) install
	ldconfig

clean:
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE)

