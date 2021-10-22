include ../../Makefile.include
PLATFORM=$(NATIVEPLATFORM)
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=ninja
VERSION=1.10.2
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz

LIBDYLIB=$(PLATFORM)/ninja

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
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
