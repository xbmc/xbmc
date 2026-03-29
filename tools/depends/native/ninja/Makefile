include ../../Makefile.include NINJA-VERSION ../../download-files.include
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile NINJA-VERSION ../../download-files.include

LIBDYLIB=$(PLATFORM)/ninja

all: .installed-$(PLATFORM)


.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	cd $(PLATFORM); CXX="$(CXX_FOR_BUILD)" AR="$(AR_FOR_BUILD)" CFLAGS="$(NATIVE_CFLAGS)" LDFLAGS="$(NATIVE_LDFLAGS)" $(NATIVEPREFIX)/bin/python3 configure.py --bootstrap

.installed-$(PLATFORM): $(LIBDYLIB)
	install $(PLATFORM)/ninja $(NATIVEPREFIX)/bin
	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
