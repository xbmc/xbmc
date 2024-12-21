include ../../Makefile.include
include LIBDATACHANNEL-VERSION
include ../../download-files.include

DEPS := \
	../../Makefile.include \
  ../../download-files.include \
  0001-Also-use-static-library-name-in-find_library.patch \
  0002-Only-install-static-library.patch \
  LIBDATACHANNEL-VERSION \
  Makefile \

CMAKE_OPTIONS := \
  -DNO_EXAMPLES=ON \
  -DNO_TESTS=ON \
  -DPREFER_SYSTEM_LIB=ON \
  $(CMAKE_OPTIONS) \

# Build directory
BUILDDIR := $(PLATFORM)/build

# Generated library
LIBDYLIB := $(BUILDDIR)/$(BYPRODUCT)

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
	cd $(PLATFORM); patch -p1 -i ../0001-Also-use-static-library-name-in-find_library.patch
	cd $(PLATFORM); patch -p1 -i ../0002-Only-install-static-library.patch
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(BUILDDIR) clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
