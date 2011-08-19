include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=liboil
VERSION=0.3.17
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

BASE_URL=http://liboil.freedesktop.org/download

# configuration settings
export PKG_CONFIG=pkg-config --define-variable prefix=$(SDKSTAGE)
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST)

LIBDYLIB=$(SOURCE)/.libs/liboil-0.3.dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j 1 -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
