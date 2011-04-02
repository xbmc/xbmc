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
TARBALLS_LOCATION=/Users/Shared/xbmc-depends/tarballs

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE=$(SOURCE).tar.bz2
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

SDK_VERSION:=$(shell xcodebuild -showsdks | grep iphoneos | sort | tail -n 1 | awk '{ print $$2}')

LIBDYLIB=$(PREFIX)/lib/libboost_thread.a

all: $(LIBDYLIB)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBDYLIB): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cat user-config.jam-ios-4.2_armv7.in > $(SOURCE)/tools/build/v2/user-config.jam
	sed -ie "s/iPhoneOS*.*.sdk/iPhoneOS$(SDK_VERSION).sdk/g" "$(SOURCE)/tools/build/v2/user-config.jam"
	cat $(SOURCE)/tools/build/v2/user-config.jam
	cd $(SOURCE); ./bootstrap.sh --prefix=$(PREFIX) --with-libraries=thread
	cd $(SOURCE); ./bjam --prefix=$(PREFIX) toolset=darwin-4.0 --architecture=arm target-os=iphone macosx-version=iphone-${SDK_VERSION} define=_LITTLE_ENDIAN install

clean:
	cd $(SOURCE); ./bjam --clean
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -f .installed

distclean::
	rm -rf $(PREFIX)/include/boost
	rm -f  $(PREFIX)/lib/libboost*
	rm -rf $(SOURCE) .installed

