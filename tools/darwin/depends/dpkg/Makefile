include ../Makefile.include

# lib name, version
APPNAME=dpkg
VERSION=1.14.30
SOURCE=$(APPNAME)_$(VERSION)
SOURCE_DEBIAN=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

export LIBTOOL=builds/unix/libtool
export PATH:=$(PREFIX)/bin:$(PATH)
CONFIGURE=./configure --prefix=$(PREFIX) \
  --enable-static --disable-shared

CLEAN_FILES=$(ARCHIVE) $(SOURCE_DEBIAN)

all: $(APPNAME) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE_DEBIAN): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(SOURCE_DEBIAN)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE_DEBIAN) > .gitignore
	cd $(SOURCE_DEBIAN); $(CONFIGURE)

$(APPNAME): $(SOURCE_DEBIAN)
	make -C $(SOURCE_DEBIAN)/lib CFLAGS="" LDFLAGS=""
	make -C $(SOURCE_DEBIAN)/libcompat CFLAGS="" LDFLAGS=""
	make -C $(SOURCE_DEBIAN)/src CFLAGS="" LDFLAGS=""
	make -C $(SOURCE_DEBIAN)/dpkg-deb CFLAGS="" LDFLAGS=""

.installed:
	make -C $(SOURCE_DEBIAN)/dpkg-deb install
	touch $@

clean:
	make -C $(SOURCE_DEBIAN) clean
	rm -f .installed

distclean::
	rm -rf $(SOURCE_DEBIAN) .installed

