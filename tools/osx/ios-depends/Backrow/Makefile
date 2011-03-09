#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

SOURCE=Backrow
# download location and format
BASE_URL=http://xbmc-for-atv2.googlecode.com/svn/trunk/Backrow


RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)

ARCHIVE=$(SOURCE).tar.gz
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

all: $(SOURCE) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(ARCHIVE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(ARCHIVE) > .gitignore
	echo $(SOURCE) >> .gitignore

.installed:
	mkdir -p $(PREFIX)/include
	cp -rf $(SOURCE) $(PREFIX)/include/
	touch $@

clean:
distclean::
	rm -rf $(SOURCE) .installed

