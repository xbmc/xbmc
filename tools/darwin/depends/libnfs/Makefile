include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=libnfs
VERSION=1.6.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(SOURCE)/lib/$(DYLIBNAME)

CLEAN_FILES=$(ARCHIVE) $(SOURCE)

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(SOURCE): $(TARBALLS_LOCATION)/$(ARCHIVE)
	-rm -rf $(SOURCE)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(SOURCE) > .gitignore
	mkdir -p $(PREFIX)/include/rpc
	cd $(SOURCE); ./bootstrap
	cd $(SOURCE); patch -p0 < ../timeval.patch
	cd $(SOURCE); $(CONFIGURE)

$(LIBDYLIB): $(SOURCE)
	make -C $(SOURCE)

.installed:
	make -C $(SOURCE) install
	touch $@
clean:
	make -C $(SOURCE) clean
	rm -r .installed
	rm -rf $(SOURCE)

distclean::
	rm -rf $(SOURCE) .installed
