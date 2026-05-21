include ../../Makefile.include XZ-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile XZ-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DXZ_NLS=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DXZ_TOOL_XZDEC=OFF \
              -DXZ_TOOL_LZMADEC=OFF \
              -DXZ_TOOL_LZMAINFO=OFF \
              -DXZ_TOOL_XZ=OFF \
              -DXZ_DOC=OFF \
              -DBUILD_TESTING=OFF

LIBDYLIB=$(PLATFORM)/build/liblzma.a

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE) $(CMAKE_OPTIONS) ..
	touch $@

$(LIBDYLIB): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
