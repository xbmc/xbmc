include ../Makefile.include

# lib name, version
BASE_URL=ftp://ftp.gnutls.org/gcrypt/gnutls/v3.1
LIBNAME=gnutls
VERSION=3.1.14
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
ARCHIVE_TOOL=/usr/local/bin/gtar

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --without-p11-kit --disable-nls

LIBDYLIB=$(SOURCE)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) xJf $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -j $(MAKE_JOBS) -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@

clean:
	make -C $(SOURCE) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE) .installed
