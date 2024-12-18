include ../../Makefile.include
include LIBJUICE-VERSION
include ../../download-files.include

DEPS := \
  ../../download-files.include \
  ../../Makefile.include \
  0001-CMake-Only-install-static-library.patch \
  LIBJUICE-VERSION  \
  Makefile \

CMAKE_OPTIONS := \
  -DNO_TESTS=ON \
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
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); patch -p1 -i ../0001-CMake-Only-install-static-library.patch
	cd $(BUILDDIR); $(CMAKE) $(CMAKE_OPTIONS) ..

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -C $(BUILDDIR)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(BUILDDIR) install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM)/build clean
	rm -f .installed-$(PLATFORM)

distclean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
