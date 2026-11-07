include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

APPNAME=nasm
VERSION=3.02
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=dc80d8a9a582423e62703da3cc3f37ee57735939b975faa8a72d061a8b596f763d206c7cc3e48c32d2ad726f38e430dc3b85cffd0c3b32e71e20ad9cc24f4804
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

APP=$(PLATFORM)/$(APPNAME)

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)
	touch $@

$(APP): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm  .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)

