include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=setuptools
VERSION=53.0.0
ARCHIVE=$(LIBNAME)-$(VERSION).tar.gz
SHA512=d045198210f09c5f2acbd487d3dd291cd7ce814bebe331f1876c133cd28f56d368717c7bd4a875b439c9cc8c9488dc9a7d3e27ab791cce419f78b87fcfd8fff6
include ../../download-files.include

all: .installed-$(PLATFORM)

	cd $(TARBALLS_LOCATION); chmod +x $(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	-rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(PREFIX)/bin/python3 bootstrap.py

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); $(PREFIX)/bin/python3 setup.py install --prefix=$(PREFIX)
	touch $@

clean:
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
