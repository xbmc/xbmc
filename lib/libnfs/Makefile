#  A quick and dirty Makefile to download/build and install
#
#  Usage:
#    make
#    sudo make install

# lib name, version
LIBNAME=libnfs
VERSION=0977fbb
SOURCE=Memphiz-$(LIBNAME)-$(VERSION)
# download location and format
BASE_URL=http://githubredir.debian.net/github/memphiz/libnfs
ARCHIVE=$(SOURCE).tar.gz
TARBALLS_LOCATION=.
RETRIEVE_TOOL=/usr/bin/curl
RETRIEVE_TOOL_FLAGS=-Ls --create-dirs --output $(TARBALLS_LOCATION)/$(ARCHIVE)
ARCHIVE_TOOL=tar
ARCHIVE_TOOL_FLAGS=xf
prefix=/usr

# configuration settings
CONFIGURE=./configure --prefix=$(prefix) --without-examples --without-ldconfig

SO_NAME=$(SOURCE)/lib/libnfs.so

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(SO_NAME)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	cd $(SOURCE);autoreconf -vif;$(CONFIGURE)

$(SO_NAME): $(SOURCE)
	make -C $(SOURCE)

install:
	make -C $(SOURCE) install
	ln -s $(prefix)/lib/libnfs.so.1.0.0 $(prefix)/lib/libnfs.so
	ln -s $(prefix)/lib/libnfs.so.1.0.0 $(prefix)/lib/libnfs.so.1
clean:
	make -C $(SOURCE) clean
	rm  -f .installed
	rm -f -r include
	rm -f -r lib
	rm -f *.tar.gz
	rm -r -f Memphiz-libnfs*
	rm -r -f nfsc

distclean::
	rm -rf $(SOURCE) .installed
