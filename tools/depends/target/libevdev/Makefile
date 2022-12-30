include ../../Makefile.include
DEPS =../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=libevdev
VERSION=1.5.8
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=e9485d8c6b48ebf762b32b003bc6b761c548024ad3bc34a0793485b53383746881fcc9b206a078d571937291e519d9822e50c5d8816778d347094691b65ec3fa
include ../../download-files.include

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX) --disable-shared --enable-static --disable-tests

LIBDYLIB=$(PLATFORM)/libevdev/.libs/libevdev.la

all: .installed-$(PLATFORM)


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

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
