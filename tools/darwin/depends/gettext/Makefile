include ../Makefile.include

# lib name, version
LIBNAME=gettext
VERSION=0.17
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
    --disable-csharp --disable-native-java --disable-java --without-emacs \
    --disable-libasprintf --disable-openmp \
    --with-included-gettext \
    --with-included-glib \
    --with-included-libcroco \
    --with-included-libxml \
    --without-git --without-cvs

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p0 <../01-gettext-tools-Makefile.in.patch
	if [ "$(DARWIN)" = "osx" ] && [ "$(SDK)" != "10.4" ] && [ "$(SDK)" != "10.5" ] && [ "$(SDK)" != "10.6" ];then \
	  cd $(SOURCE); patch -p1 <../02-gettext-tools-stpncpy.patch; \
	fi
	if [ "$(DARWIN)" = "ios" ] && [ "$(SDK)" != "4.2" ]; then \
	  cd $(SOURCE); patch -p1 <../02-gettext-tools-stpncpy.patch; \
	fi
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
