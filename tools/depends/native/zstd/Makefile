include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

LIBNAME=zstd
VERSION=1.5.5
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=99109ec0e07fa65c2101c9cb36be56b672bbd0ee69d265f924718e61f9192ae8385c8d9e4d0c318be9edfa6d849fd3d60e5f164fa120961449429ea3c5dab6b6
include ../../download-files.include

all: .installed-$(PLATFORM)

$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/lib libzstd.a PREFIX=$(PREFIX)
	$(MAKE) -C $(PLATFORM)/lib install-pc install-static install-includes PREFIX=$(PREFIX)
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

