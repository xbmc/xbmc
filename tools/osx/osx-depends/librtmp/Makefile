#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=rtmpdump
VERSION=2.3
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://rtmpdump.mplayerhq.hu/download
ARCHIVE=$(SOURCE).tgz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup
export LDFLAGS+=-Wl,-read_only_relocs,suppress

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore
	cd $(SOURCE); patch -p1 < ../01-make_shared_lib_for_darwin-tag2.3.patch

$(LIBDYLIB): $(SOURCE)
	make SYS=darwin prefix=$(PREFIX) XCFLAGS="$(CFLAGS)" XLDFLAGS="$(LDFLAGS)" -C $(SOURCE)/librtmp

.installed:
	make SYS=darwin prefix=$(PREFIX) -C $(SOURCE)/librtmp install
	cp $(PREFIX)/lib/librtmp.so ../../../../system/
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f ../../../../system/librtmp.so
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
