include ../../Makefile.include
include PLOG-VERSION
include ../../download-files.include

DEPS := \
  ../../download-files.include \
  ../../Makefile.include \
  Makefile \
  PLOG-VERSION \

CMAKE_OPTIONS := \
  -DPLOG_BUILD_SAMPLES=OFF \
  -DPLOG_INSTALL=ON \
  $(CMAKE_OPTIONS) \

# Build directory
BUILDDIR := $(PLATFORM)/build

.PHONY: .installed-native

all: .installed-$(PLATFORM)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE).$(HASH_TYPE) $(DEPS)
ifeq ($(PREFIX),)
	@echo
	@echo "ERROR: please set PREFIX to the kodi install path e.g. make PREFIX=/usr/local"
	@exit 1
endif
	rm -rf $(PLATFORM); mkdir -p $(BUILDDIR)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
