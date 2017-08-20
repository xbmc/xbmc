include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile python3.patch

# lib name, version
LIBNAME=distutilscross
VERSION=0.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../python3.patch

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); $(PREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
