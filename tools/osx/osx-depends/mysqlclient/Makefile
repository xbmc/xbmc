#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

include ../config.mk

LIBNAME=mysql
VERSION=5.1.55
SOURCE=$(LIBNAME)-$(VERSION)
# download location and format
#BASE_URL=http://mirrors.xbmc.org/build-deps/darwin-libs
BASE_URL=http://mysql.mirrors.pair.com/Downloads/MySQL-5.1
ARCHIVE=$(SOURCE).tar.gz

RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --output $(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf

# configuration settings
export CFLAGS+=-undefined dynamic_lookup
export CXXFLAGS+=-undefined dynamic_lookup
export LDFLAGS+=-Wl,-read_only_relocs,suppress
CONFIGURE=./configure --prefix=$(PREFIX) --host=$(HOST) --enable-shared \
  --enable-thread-safe-client -with-extra-charsets=complex \
  --without-readline --without-server --without-bench --without-docs --without-man \
  ac_cv_func_locking=no \
  ac_cv_func_thr_setconcurrency=no \
  ac_cv_func_clock_gettime=no \
  ac_cv_func_gethrtime=no \
  ac_cv_func_printstack=no \
  ac_cv_func_backtrace=no \
  ac_cv_func_backtrace_symbols=no \
  ac_cv_func_gethostbyname_r=no \
  ac_cv_func_bfill=no \
  ac_cv_func_bmove=no \
  ac_cv_func_bsearch=no \
  ac_cv_func_bzero=no \
  ac_cv_func_fconvert=no \

LIBDYLIB=$(SOURCE)/libmysql/.libs/$(LIBNAME).dylib

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
	make -C $(SOURCE)/scripts install
	make -C $(SOURCE)/include install
	make -C $(SOURCE)/libmysql install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
