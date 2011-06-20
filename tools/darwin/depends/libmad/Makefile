include ../Makefile.include

# lib name, version
LIBNAME=libmad
VERSION=0.15.1b
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
export PATH:=$(TOOLCHAIN)/bin:${PREFIX}/bin:/Developer/usr/bin:${PATH}
CONFIGURE=./configure --prefix=$(PREFIX) \
  --enable-fpm=default

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); patch -p1 < ../01-libmad-pkgconfig.patch
	if test "$(DARWIN)" = "ios"; then \
		cd $(SOURCE); patch -p1 < ../02-libmad-thumb-fix.patch; \
	fi
	sed -ie "s|libmad.list|libmad.list mad.pc|g" "$(SOURCE)/configure.ac"
	cd $(SOURCE); touch NEWS
	cd $(SOURCE); touch AUTHORS
	cd $(SOURCE); touch ChangeLog
	cd $(SOURCE); autoreconf -vi
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
