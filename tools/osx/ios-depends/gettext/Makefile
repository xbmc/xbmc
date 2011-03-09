#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

# lib name, version
LIBNAME=gettext
VERSION=0.17
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
ARCHIVE=$(SOURCE).tar.gz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup
export LDFLAGS+=-Wl,-read_only_relocs,suppress -Wl,-undefined,dynamic_lookup
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared --disable-csharp --disable-native-java --without-emacs --disable-java --disable-libasprintf --disable-csharp --disable-openmp --without-emacs --with-included-gettext --without-git --without-cvs ac_cv_func___fsetlocking=no

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(ARCHIVE)
	echo $(ARCHIVE) > .gitignore
	echo $(SOURCE) >> .gitignore
	cd $(SOURCE); patch -p0 <../01-gettext-tools-Makefile.in.patch
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
