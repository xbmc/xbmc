include ../../Makefile.include
DEPS = ../../Makefile.include Makefile armAssembler.patch ../../download-files.include

# lib name, version
LIBNAME=libffi
VERSION=3.2.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=980ca30a8d76f963fca722432b1fe5af77d7a4e4d2eac5144fbc5374d4c596609a293440573f4294207e1bdd9fda80ad1e1cafb2ffb543df5a275bc3bd546483
include ../../download-files.include

# configuration settings
CONFIGURE= ./configure --prefix=$(PREFIX) --disable-shared --disable-builddir


LIBDYLIB=$(PLATFORM)/.libs/$(LIBNAME).a

all: .installed-$(PLATFORM)


$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); patch -p1 -i ../armAssembler.patch
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
	mkdir -p $(PREFIX)/include/ffi
	cp $(PREFIX)/lib/$(SOURCE)/include/* $(PREFIX)/include/ffi/
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
