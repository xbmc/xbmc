include ../../Makefile.include NV-CODEC-HEADERS-VERSION ../../download-files.include
DEPS = ../../Makefile.include Makefile NV-CODEC-HEADERS-VERSION ../../download-files.include

# Header-only, FFmpeg and Kodi load CUDA and NVDEC from the driver at runtime

all: .installed-$(PLATFORM)

.configured-$(PLATFORM): $(DEPS) | $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	touch $@

.installed-$(PLATFORM): .configured-$(PLATFORM)
	$(MAKE) -C $(PLATFORM) PREFIX=$(PREFIX) install
	touch $@

clean:
	rm -f .installed-$(PLATFORM) .configured-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM) .configured-$(PLATFORM)
