include ../../Makefile.include SWIG-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)

DEPS = ../../Makefile.include Makefile SWIG-VERSION ../../download-files.include

LIBDYLIB=$(PLATFORM)/build/swig

all: .installed-$(PLATFORM)


$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE_FOR_BUILD) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)


distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

