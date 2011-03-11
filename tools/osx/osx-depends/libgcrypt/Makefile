#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=libgcrypt
VERSION=1.4.0
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=ftp://ftp.gnupg.org/gcrypt/libgcrypt
ARCHIVE=$(SOURCE).tar.bz2

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup
export LDFLAGS+=-Wl,-read_only_relocs,suppress
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared=yes \
  ac_cv_func_gethrtime=no

LIBDYLIB=$(SOURCE)/.libs/libgcrypt.dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)
	echo $(ARCHIVE) > .gitignore

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(SOURCE) >> .gitignore
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

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

