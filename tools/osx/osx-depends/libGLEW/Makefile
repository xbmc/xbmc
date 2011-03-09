#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=glew
VERSION=1.5.8
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
#BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
BASE_URL=http://voxel.dl.sourceforge.net/project/glew/glew/1.5.8
ARCHIVE=$(SOURCE).tgz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
#export CFLAGS+=-undefined dynamic_lookup 
#export LDFLAGS+=-Wl,-read_only_relocs,suppress
export GLEW_DEST=$(PREFIX)
export CC.ARCH=/usr/bin/gcc-4.0
export CFLAGS.ARCH=$(CFLAGS)
export LDFLAGS.ARCH=$(LDFLAGS)

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
	cd $(SOURCE); patch -p0 < ../01-Makefile.patch
	cd $(SOURCE); patch -p0 < ../02-Makefile.darwin.patch

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm  .installed

distclean::
	rm -rf $(SOURCE) .installed
