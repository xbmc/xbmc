include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=meson
VERSION=1.0.0
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=9b1195cfe856c1aa51bc79f6eb4d0f94925bb02d0a9fbd68a6a6ced6e5c252b09b22d9aac812640687e49b8d64a313ce48d0a69a3bf83ea8ffb8c9dab559fc23
include ../../download-files.include

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py config

.installed-$(PLATFORM): $(PLATFORM)
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py build
	cd $(PLATFORM); $(NATIVEPREFIX)/bin/python3 setup.py install --prefix="$(NATIVEPREFIX)"
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
