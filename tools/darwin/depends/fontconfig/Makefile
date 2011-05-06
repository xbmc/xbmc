include ../Makefile.include

# lib name, version
LIBNAME=fontconfig
VERSION=2.6.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings 
export PATH:=$(TOOLCHAIN)/bin:$(PATH) 
CONFIGURE=./configure --prefix=$(PREFIX) \
  --disable-libxml2 --disable-docs \
  --with-freetype-config=$(PREFIX)/bin/freetype-config

LIBDYLIB=$(SOURCE)/.libs/lib$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p1 < ../01-fontconfig-cross-compile-fix.patch
	cd $(SOURCE); autoreconf -vif
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
