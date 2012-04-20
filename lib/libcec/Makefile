#  Download and build libCEC
#
#  Usage:
#    make
#    sudo make install
#    make clean

# lib name, version
LIBNAME=libcec
VERSION=latest
SOURCE=$(LIBNAME)-$(VERSION)

# download location and format
BASE_URL=http://packages.pulse-eight.net/pulse/sources/libcec
ARCHIVE=$(SOURCE).tar.bz2 
TARBALLS_LOCATION=.
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=jxf

PREFIX ?= /usr/local
LIBCEC_CONFIGOPTS ?= --prefix=$(PREFIX)

# configuration settings
CONFIGURE=./configure CFLAGS=-D_FILE_OFFSET_BITS=64 $(LIBCEC_CONFIGOPTS)

SO_NAME=$(LIBNAME)/.libs/$(LIBNAME).so

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBNAME): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(LIBNAME)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(LIBNAME) > .gitignore
	cd $(LIBNAME); autoreconf -vif
	cd $(LIBNAME); $(CONFIGURE)

$(SO_NAME): $(LIBNAME)
	make -j 1 -C $(LIBNAME)

install:
	make -C $(LIBNAME) install
	ldconfig

clean:
	rm -rf $(LIBNAME)

distclean::
	rm -rf $(LIBNAME)

