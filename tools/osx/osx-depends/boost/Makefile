#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

LIBNAME=boost
VERSION=1_44_0
SOURCE=$(LIBNAME)_$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE=$(SOURCE).tar.bz2
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

LIBDYLIB=$(PREFIX)/lib/libboost_thread-mt.a

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore
	cat user-config.jam-osx-10.4_i386.in >> $(SOURCE)/tools/build/v2/user-config.jam

$(LIBDYLIB): $(SOURCE)
	cd $(SOURCE); ./bootstrap.sh --prefix=$(PREFIX) --with-libraries=thread
	cd $(SOURCE); ./bjam --prefix=$(PREFIX) toolset=darwin-4.0 --architecture=x86 address-model=32 target-os=darwin macosx-version=10.4 macosx-version-min=10.4 --link=static  install

.installed:
	touch $@

clean:
	cd $(SOURCE); ./bjam --clean
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -f .installed

distclean::
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -rf $(SOURCE) .installed

