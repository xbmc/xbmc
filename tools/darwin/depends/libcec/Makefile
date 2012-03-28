include ../Makefile.include

# lib name, version
LIBNAME=libcec
VERSION=1.5.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

LIBDYLIB=$(LIBNAME)/.libs/$(LIBNAME).dylib

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBNAME): $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(LIBNAME)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(LIBNAME) > .gitignore
	cd $(LIBNAME); autoreconf -vif
	cd $(LIBNAME); $(CONFIGURE)

$(LIBDYLIB): $(LIBNAME)
	make -j 1 -C $(LIBNAME)

.installed:
	make -C $(LIBNAME) install
	touch $@

clean:
	rm -rf $(LIBNAME) .installed

distclean::
	rm -rf $(LIBNAME) .installed

