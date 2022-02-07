include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=meson
VERSION=0.59.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=126ac3a6c6b9e1fba1b3ac163f02d1eb0b61fedb312bcfe4996f6150522688d424f47283070c95101cc456afe9ea5cb462fb38f368d0c732952ffb8c600fda00
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/build

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py config

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py build

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install --prefix="$(NATIVEPREFIX)"
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
