include ../Makefile.include

# lib name, version
LIBNAME=glib
VERSION=2.24.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

BASE_URL=http://ftp.acc.umu.se/pub/gnome/sources/glib/2.24

# configuration settings
# convert filenames from locale encoding to UTF-8
#export G_BROKEN_FILENAMES=1
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-gtk-doc \
  --disable-gtk-doc-html \
  --disable-gtk-doc-pdf \
  --disable-man \
  glib_cv_stack_grows=no \
  glib_cv_rtldglobal_broken=no \
  glib_cv_uscore=no \
  ac_cv_lib_dld_shl_load=yes \
  ac_cv_func_posix_getpwuid_r=yes \
  ac_cv_func_posix_getgrgid_r=yes \
  ac_cv_path_GLIB_GENMARSHAL=`pwd`/../glib-genmarshal

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	if test "$(DARWIN)" = "ios"; then \
		sed -i "" -e "s|-framework Carbon| |" "$(SOURCE)/configure"; \
	fi
	cd $(SOURCE); $(CONFIGURE)
	if test "$(DARWIN)" = "ios"; then \
		sed -i "" -e "s|#define HAVE_CARBON 1| |" "$(SOURCE)/config.h"; \
		sed -i "" -e "s|defined(USE_LIBICONV_GNU)|!defined(__APPLE__) \&\& defined(USE_LIBICONV_GNU)|g" "$(SOURCE)/glib/gconvert.c"; \
	fi

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
