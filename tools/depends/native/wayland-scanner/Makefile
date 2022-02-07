include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS =../../Makefile.include Makefile ../../download-files.include

APPNAME=wayland-scanner
PROJECTNAME=wayland
VERSION=1.17.0
SOURCE=$(PROJECTNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=c5051aab5ff078b368c196ecfedb33ccd961265bb914845d7ed81de361bb86ae18299575baa6c4eceb0d82cf8b495e8293f31b51d1cbc05d84af0a199ab3f946
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-libraries --disable-documentation --disable-dtd-validation

APP=$(PLATFORM)/wayland-scanner

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(APP): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
