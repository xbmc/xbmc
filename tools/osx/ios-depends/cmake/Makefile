#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# lib name, version
APPNAME=cmake
VERSION=2.8.4
SOURCE=$(APPNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.gz
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
PREFIX:=/Users/Shared/xbmc-depends/ios-4.2_arm7
export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./bootstrap --prefix=$(PREFIX) --host=$(HOST)

APP=$(SOURCE)/$(APPNAME)

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(APP) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(APP): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm  .installed

distclean::
	rm -rf $(SOURCE) .installed
