include ../../Makefile.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile visibility.patch ../../download-files.include

# lib name, version
LIBNAME=zlib
VERSION=1.2.13
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.xz
SHA512=9e7ac71a1824855ae526506883e439456b74ac0b811d54e94f6908249ba8719bec4c8d7672903c5280658b26cb6b5e93ecaaafe5cdc2980c760fa196773f0725
include ../../download-files.include
# configuration settings
CONFIGURE= CC="$(CC_FOR_BUILD)" CFLAGS="$(NATIVE_CFLAGS)" ./configure --prefix=$(PREFIX)

LIBDYLIB=$(PLATFORM)/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../visibility.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
