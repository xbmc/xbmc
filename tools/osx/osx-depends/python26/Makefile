#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install
#

include ../config.mk

# lib name, version
LIBNAME=Python
VERSION=2.6.5
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.bz2
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export EXTRA_CFLAGS=$(CFLAGS)
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared \
  --disable-toolbox-glue --enable-unicode=ucs4

LIBDYLIB=$(SOURCE)/libpython2.6.dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)
	# python2.6 has an issue detecting and using the same version of openssl in configure and setup.py
	# this forces python2.6 hashlib to be compatible with osx 10.4 boxes.
	sed -ie "s|openssl_ver >= 0x00907000)|openssl_ver >= 0x00907000 and False)|" "$(SOURCE)/setup.py"
	sed -ie "s|(openssl_ver < 0x00908000)|(True or openssl_ver < 0x00908000)|" "$(SOURCE)/setup.py"
	cd $(SOURCE); make

.installed:
	cd $(SOURCE); make install
	find $(PREFIX)/lib/python2.6 -type f -name *.pyc -exec rm -f {} \;	
	find $(PREFIX)/lib/python2.6 -type f -name *.pyo -exec rm -f {} \;	
	touch $@

clean:
	rm -rf $(SOURCE) .installed

distclean::
	rm -rf $(SOURCE) .installed

