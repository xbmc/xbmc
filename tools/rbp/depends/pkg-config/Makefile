include ../../Makefile.include

# lib name, version
# lib name, version
LIBNAME=pkg-config
VERSION=0.23
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
PREFIX:=$(XBMCPREFIX)
export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) --enable-indirect-deps --with-pc-path=$(PREFIX)/lib/pkgconfig:$(PREFIX)/share/pkgconfig

LIBDYLIB=$(SOURCE)/bin/pkg-config

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j $(JOBS) -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
