#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# app name, version
APPNAME=tar
VERSION=1.23
SOURCE=$(APPNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
#BASE_URL=http://ftp.gnu.org/gnu/tar
ARCHIVE=$(SOURCE).tar.gz
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

export LIBTOOL=builds/unix/libtool
PREFIX:=/Users/Shared/xbmc-depends/ios-4.2_arm7
CONFIGURE=./configure --prefix=$(PREFIX) \
--program-transform-name=s/tar/gtar/ --disable-dependency-tracking

CLEAN_FILES=$(ARCHIVE) $(SOURCE)/src/tar

all: $(SOURCE)/src/tar .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(SOURCE)/src/tar: $(SOURCE)
	cd $(SOURCE); make

.installed:
	make -C $(SOURCE) install
	cd $(PREFIX)/bin; ln -s gtar tar; ln -s gtar gnutar
	touch $@

clean:
	make -C $(SOURCE) clean
	rm  .installed

distclean::
	rm -rf $(SOURCE) .installed

