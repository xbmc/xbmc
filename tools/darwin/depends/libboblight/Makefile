include ../Makefile.include
include ../config.site.mk

#hint for building a fat lib - "lipo -arch i386 libboblight-i386.dylib -arch x86_64 libboblight-x86_64.dylib -output libboblight-fat.dylib"

# lib name, version
LIBNAME=libboblight
VERSION=r429
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --without-opengl \
  --without-portaudio \
  --without-x11

LIBDYLIB=$(SOURCE)/src/.libs/$(LIBNAME).dylib

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); autoreconf -vif
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	echo "libboblight isn't a dependency of XBMC and won't be installed"
	touch $@
clean:
	make -C $(SOURCE) clean
	rm -r .installed
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE) .installed
