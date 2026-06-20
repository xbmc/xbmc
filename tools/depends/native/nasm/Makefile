include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

APPNAME=nasm
VERSION=3.01
SOURCE=$(APPNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=ca9550ad3239cfaa9546a908e8de86e11a7c7dd9d6006f42d272d0b025b601cdb205e60b8b3929f31123ef450dd0f0403e1300328cadf0ffdfd459f443944482
FULL_URL=https://www.nasm.us/pub/nasm/releasebuilds/3.01/nasm-3.01.tar.xz
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

