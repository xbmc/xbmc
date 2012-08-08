NATIVE_BUILD=1
include ../Makefile.include
DEPS= ../Makefile.include Makefile

# lib name, version
LIBNAME=Python
VERSION=2.6.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
CWD=$(shell pwd)
HOSTPYTHONDIR=$(CWD)/$(PLATFORM)/hostpython

CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --disable-toolbox-glue --disable-framework

LIBDYLIB=$(PLATFORM)/python

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(MAKE)

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); make install
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
