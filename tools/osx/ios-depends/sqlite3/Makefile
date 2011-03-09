#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=sqlite
VERSION=3.6.11
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
#BASE_URL=http://www.sqlite.org
ARCHIVE=$(SOURCE).tar.gz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup -DSQLITE_ENABLE_COLUMN_METADATA=1
export LDFLAGS+=-Wl,-read_only_relocs,suppress
export TCLLIBDIR=/dev/null
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared \
  --enable-threadsafe --disable-tcl \
  --with-readline-inc="-I${PREFIX}/include" \
  --with-readline-lib="-L${PREFIX}/lib -lreadline -lm -lncurses"

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore
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
