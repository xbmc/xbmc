#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

SOURCE=Backrow
# download location and format
BASE_URL=http://xbmc-for-atv2.googlecode.com/svn/trunk/Backrow

TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)

ARCHIVE=$(SOURCE).tar.gz
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

all: $(SOURCE) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore

.installed:
	mkdir -p $(PREFIX)/include
	cp -rf $(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -rf $(SOURCE) .installed

