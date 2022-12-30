include ../../Makefile.include
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=dbus
VERSION=1.11.16
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=991ba6293d5f305e5a7e4ce6b131ea7fddd3bebadbb5b8c2fec29ccb76ab324786a01b89d2df6a3a112756436549dc7b856e78d4adb3f7514c8537ad854741bc
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) \
  --without-x --disable-xml-docs --disable-doxygen-docs

LIBDYLIB=$(PLATFORM)/$(LIBNAME)/.libs/lib$(LIBNAME)-1.so

all: $(LIBDYLIB) .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
