#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

LIBNAME=mysql
VERSION=5.0.89
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE)-ios.tar.gz
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

all: $(SOURCE) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore

.installed:
	cp -rf $(SOURCE)/* $(PREFIX)
	install_name_tool -id $(PREFIX)/lib/mysql/libmysqlclient.15.dylib $(PREFIX)/lib/mysql/libmysqlclient.dylib
	touch $@

clean:
distclean::
	rm -rf $(SOURCE) .installed

