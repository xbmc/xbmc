include ../../Makefile.include XZ-VERSION ../../download-files.include
PREFIX=$(NATIVEPREFIX)
PLATFORM=$(NATIVEPLATFORM)
DEPS = ../../Makefile.include Makefile XZ-VERSION ../../download-files.include

# configuration settings
CMAKE_OPTIONS=-DXZ_NLS=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              -DXZ_TOOL_LZMAINFO=OFF \
              -DXZ_DOC=OFF \
              -DBUILD_TESTING=OFF

APP=$(PLATFORM)/build/xz

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)/build
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM)/build; $(CMAKE_FOR_BUILD) $(CMAKE_OPTIONS) ..
	touch $@

$(APP): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM)/build

.installed-$(PLATFORM): $(APP)
	$(MAKE) -C $(PLATFORM)/build install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
