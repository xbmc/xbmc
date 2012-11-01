include ../Makefile.buildtools
export NASM=$(TOOLCHAIN)/bin/yasm
export CFLAGS=-I$(TOOLCHAIN)/include -fexceptions
export LDFLAGS=-L$(TOOLCHAIN)/lib
export CPPFLAGS=-I$(TOOLCHAIN)/include
export CXXFLAGS=-I$(TOOLCHAIN)/include

# lib name, version
LIBNAME=libjpeg-turbo
VERSION=1.2.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(TOOLCHAIN) \
  --with-jpeg8

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); autoreconf -vif
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
