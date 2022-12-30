include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile ../../download-files.include

# lib name, version
LIBNAME=ninja
VERSION=1.10.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
SHA512=895412ae1cbc83c656e92f282602a29300e08274e9dea0da4464202ae556e7a1ab03bec057f23da4756bbd91bd2d744cd7a64b336740fd2782bb4db5c3b7b496
include ../../download-files.include

LIBDYLIB=$(PLATFORM)/ninja

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM); CXX="$(CXX_FOR_BUILD)" AR="$(AR_FOR_BUILD)" CFLAGS="$(NATIVE_CFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)" $(NATIVEPREFIX)/bin/python3 configure.py --bootstrap

.installed-$(PLATFORM): $(LIBDYLIB)
	install $(PLATFORM)/ninja $(NATIVEPREFIX)/bin
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
