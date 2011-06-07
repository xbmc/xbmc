#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=lloyd-yajl
VERSION=1.0.12
SOURCE=lloyd-yajl-17b1790
ARCHIVE=lloyd-yajl-1.0.12-0-g17b1790.tar.gz
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(SOURCE)/build/yajl-1.0.12/lib/libyajl.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	sed -ie "s|ADD_CUSTOM_TARGET|#ADD_CUSTOM_TARGET|g" "$(SOURCE)/CMakeLists.txt"
	sed -ie "s|./run_tests.sh|#./run_tests.sh|g" "$(SOURCE)/CMakeLists.txt"
	sed -ie "s|WORKING_DIRECTORY|#WORKING_DIRECTORY|g" "$(SOURCE)/CMakeLists.txt"
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
