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
# git clone git://gitorious.org/boostoniphone/boostoniphone.git
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.bz2
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

LIBDYLIB=$(SOURCE)/prefix/lib/libboost_system.a

all: $(LIBDYLIB) .compiled .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)

.compiled:
	rm -rf src build prefix framework
	echo src > .gitignore
	echo build >> .gitignore
	echo prefix >> .gitignore
	echo framework >> .gitignore
	sh ./boost.sh
	touch $@

.installed:
	cp -rf prefix/* $(PREFIX) 
	touch $@

clean:
distclean::
	rm -rf src build prefix framework .compiled .installed

